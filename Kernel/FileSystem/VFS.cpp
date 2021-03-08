#include "VFS.h"

#include "Drivers/DeviceManager.h"
#include "Drivers/Storage.h"
#include "FAT32.h"

namespace kernel {

VFS* VFS::s_instance;

VFS::VFS()
{
    auto storage_devices = DeviceManager::the().all_of(Device::Category::STORAGE);

    for (auto device : storage_devices)
        load_all_partitions(*static_cast<StorageDevice*>(device));
}

void VFS::load_all_partitions(StorageDevice& device)
{
    auto info = device.query_info();
    if (info.lba_size != 512 && info.lba_size != 4096)
    {
        warning() << "VFS: device " << device.device_model() << " has an unsupported lba size of " << info.lba_size;
        return;
    }

    auto lba0_region = MemoryManager::the().allocate_kernel_private_anywhere("LBA0"_sv, Page::size);
    auto& lba0 = *static_cast<PrivateVirtualRegion*>(lba0_region.get());
    lba0.preallocate_range();
    
    auto lba_count = Page::size / info.lba_size;
    ASSERT(lba_count != 0); // just in case lba size is somehow greater than page size

    device.read_synchronous(lba0.virtual_range().begin(), { 0, lba_count });

    static constexpr StringView gpt_signature = "EFI PART"_sv;
    static constexpr size_t offset_to_gpt_signature = 512;
    auto lba0_data = lba0.virtual_range().begin().as_pointer<char>();

    if (StringView(lba0_data + offset_to_gpt_signature, gpt_signature.size()) == gpt_signature) {
        warning() << "VFS: detected a GPT-partitioned device " << device.device_model()
                  << " which is currently not supported";
        MemoryManager::the().free_virtual_region(*lba0_region);
        return;
    }

    static constexpr u16 mbr_partition_signature = 0xAA55;
    static constexpr size_t offset_to_mbr_signature = 510;
    auto signature = *Address(lba0_data + offset_to_mbr_signature).as_pointer<u16>();
    if (signature != mbr_partition_signature) {
        warning() << "VFS: device " << device.device_model() << " contains an invalid MBR signature "
                  << format::as_hex << "(expected: " << mbr_partition_signature << " got: " << signature;
        MemoryManager::the().free_virtual_region(*lba0_region);
        return;
    }

    log() << "VFS: detected an MBR formatted storage device " << device.device_model();
    load_mbr_partitions(device, lba0.virtual_range().begin());

    // TODO: set boot FS

    MemoryManager::the().free_virtual_region(*lba0_region);
}

void VFS::load_mbr_partitions(StorageDevice& device, Address virtual_mbr)
{
    static constexpr size_t offset_to_partitions = 0x01BE;
    virtual_mbr += offset_to_partitions;

    static constexpr u8 empty_partition_type = 0x00;
    static constexpr u8 lba_fat_32_partition_type = 0x0C;

    struct PACKED MBRPartitionEntry {
        u8 status;
        u8 chs_begin[3];
        u8 type;
        u8 chs_end[3];
        u32 first_lba;
        u32 lba_count;
    };

    auto* partition = virtual_mbr.as_pointer<MBRPartitionEntry>();

    for (size_t i = 0; i < 4; ++i, partition++) {
        if (partition->type == empty_partition_type) {
            log() << "Partition " << i << " empty";
            continue;
        }

        switch (partition->type) {
        case lba_fat_32_partition_type:
            log() << "Partition " << i << " FAT32 LBA mode, starting lba: "
                  << partition->first_lba << " length: " << partition->lba_count;
            m_prefix_to_fs[generate_prefix(device)] = new FAT32(device, { partition->first_lba, partition->lba_count });
            break;
        default:
            log() << "Partition " << i << " has unknown type " << format::as_hex << partition->type << ", skipped";
        }
    }
}

String VFS::generate_prefix(StorageDevice& device)
{
    auto allocate_prefix = [this](StorageDevice::Info::MediumType medium) {
        auto idx = static_cast<size_t>(medium);
        ASSERT(idx < static_cast<size_t>(StorageDevice::Info::MediumType::LAST));

        return m_prefix_indices[idx]++;
    };

    auto info = device.query_info();

    String prefix(info.medium_type_to_string());
    prefix += allocate_prefix(info.medium_type);

    log("VFS") << "allocated new prefix " << prefix.to_view();

    return prefix;
}

Optional<Pair<StringView, StringView>> VFS::split_prefix_and_path(StringView path)
{
    Pair<StringView, StringView> prefix_to_path;

    auto pref = path.find("::");

    if (pref) {
        prefix_to_path.set_first(StringView(path.data(), pref.value()));
        prefix_to_path.set_second(StringView(path.data() + pref.value(), path.size() - pref.value()));
    } else {
        prefix_to_path.set_second(path);
    }

    auto validate_prefix = [] (StringView prefix)
    {
        if (prefix.empty())
            return true;

        for (char c : prefix) {
            if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')))
                return false;
        }

        return true;
    };

    auto validate_path  = [] (StringView path)
    {
        if (path.empty())
            return false;

        if (!path.starts_with("/"_sv))
            return false;

        static constexpr char valid_decoy = 'a';

        for (size_t i = 0; i < path.size(); ++i) {
            char lhs = i ? path[i - 1] : valid_decoy;
            char rhs = i < path.size() - 1 ? path[i + 1] : valid_decoy;
            char cur = path[i];

            if (cur == '/' && (lhs == '/' || rhs == '/'))
                return false;

            if (cur == '.' && (lhs == '.' && rhs == '.'))
                return false;
        }

        return true;
    };

    if (!validate_prefix(prefix_to_path.first()))
        return {};
    if (!validate_path(path))
        return {};

    return prefix_to_path;
}

bool VFS::is_valid_path(StringView path)
{
    return split_prefix_and_path(path).has_value();
}

RefPtr<FileDescription> VFS::open(StringView path, FileDescription::Mode mode)
{
    auto split_path = split_prefix_and_path(path);

    if (!split_path)
        return {};

    auto& prefix_to_path = split_path.value();

    auto fs = m_prefix_to_fs.find(prefix_to_path.first());

    if (fs == m_prefix_to_fs.end())
        return {};

    auto* file = fs->second()->open(prefix_to_path.second());
    if (!file)
        return {};

    return RefPtr<FileDescription>::create(*file, mode);
}

void VFS::close(FileDescription& fd)
{
    auto& file = fd.raw_file();
    file.fs().close(file);

    fd.mark_as_closed();
}

bool VFS::remove(StringView path)
{
    auto split_path = split_prefix_and_path(path);

    if (!split_path)
        return false;

    auto& prefix_to_path = split_path.value();

    auto fs = m_prefix_to_fs.find(prefix_to_path.first());

    if (fs == m_prefix_to_fs.end())
        return false;

    return fs->second()->remove(prefix_to_path.second());
}

void VFS::create(StringView file_path, File::Attributes attributes)
{
    auto split_path = split_prefix_and_path(file_path);

    if (!split_path)
        return;

    auto& prefix_to_path = split_path.value();

    auto fs = m_prefix_to_fs.find(prefix_to_path.first());

    if (fs == m_prefix_to_fs.end())
        return;

    fs->second()->create(prefix_to_path.second(), attributes);
}

void VFS::move(StringView path, StringView new_path)
{
    auto split_path_old = split_prefix_and_path(path);
    auto split_path_new = split_prefix_and_path(new_path);

    if (!split_path_old || !split_path_new)
        return;
    
    auto& prefix_to_path_old = split_path_old.value();
    auto& prefix_to_path_new = split_path_new.value();

    if (prefix_to_path_new.first() != prefix_to_path_old.first()) {
        warning() << "VFS: tried to move files cross FS, currently not implemented :(";
        return;
    }

    auto fs = m_prefix_to_fs.find(prefix_to_path_new.first());

    if (fs == m_prefix_to_fs.end())
        return;

    fs->second()->move(prefix_to_path_old.second(), prefix_to_path_new.second());
}

void VFS::copy(StringView path, StringView new_path)
{
    auto split_path_old = split_prefix_and_path(path);
    auto split_path_new = split_prefix_and_path(new_path);

    if (!split_path_old || !split_path_new)
        return;

    auto& prefix_to_path_old = split_path_old.value();
    auto& prefix_to_path_new = split_path_new.value();

    if (prefix_to_path_new.first() != prefix_to_path_old.first()) {
        warning() << "VFS: tried to copy files cross FS, currently not implemented :(";
        return;
    }

    auto fs = m_prefix_to_fs.find(prefix_to_path_new.first());

    if (fs == m_prefix_to_fs.end())
        return;

    fs->second()->copy(prefix_to_path_old.second(), prefix_to_path_new.second());
}

}