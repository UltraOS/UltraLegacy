#include "VFS.h"

#include "Drivers/DeviceManager.h"
#include "Drivers/Storage.h"
#include "FAT32/FAT32.h"
#include "Multitasking/Sleep.h"
#include "Utilities.h"

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
    if (info.logical_block_size != 512 && info.logical_block_size != 4096) {
        warning() << "VFS: device " << device.device_model() << " has an unsupported block size of " << info.logical_block_size;
        return;
    }

    auto lba0_region = MemoryManager::the().allocate_dma_buffer("LBA0"_sv, Page::size);

    auto lba_count = Page::size / info.logical_block_size;

    auto request = StorageDevice::AsyncRequest::make_read(lba0_region->virtual_range().begin(), { 0, lba_count });
    device.submit_request(request);
    request.wait();

    static constexpr StringView gpt_signature = "EFI PART"_sv;
    static constexpr size_t offset_to_gpt_signature = 512;
    auto lba0_data = lba0_region->virtual_range().begin().as_pointer<char>();

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
                  << format::as_hex << "(expected: " << mbr_partition_signature << " got: " << signature << ")";
        MemoryManager::the().free_virtual_region(*lba0_region);
        return;
    }

    log() << "VFS: detected an MBR formatted storage device " << device.device_model();
    load_mbr_partitions(device, lba0_region->virtual_range().begin());

    MemoryManager::the().free_virtual_region(*lba0_region);
}

void VFS::load_mbr_partitions(StorageDevice& device, Address virtual_mbr, size_t sector_offset)
{
    static constexpr size_t offset_to_partitions = 0x01BE;
    virtual_mbr += offset_to_partitions;

    static constexpr u8 empty_partition_type = 0x00;
    static constexpr u8 lba_fat_32_partition_type = 0x0C;
    static constexpr u8 chs_fat_32_partition_type = 0x0B;
    static constexpr u8 ebr_partition_type = 0x05;

    struct PACKED MBRPartitionEntry {
        u8 status;
        u8 chs_begin[3];
        u8 type;
        u8 chs_end[3];
        u32 first_block;
        u32 block_count;
    };

    auto* partition = virtual_mbr.as_pointer<MBRPartitionEntry>();

    for (size_t i = 0; i < 4; ++i, partition++) {
        if (partition->type == empty_partition_type) {
            log() << "Partition " << i << " empty";
            continue;
        }

        RefPtr<FileSystem> fs {};

        switch (partition->type) {
        case chs_fat_32_partition_type:
        case lba_fat_32_partition_type: {
            auto first_block = partition->first_block + sector_offset;

            log() << "Partition " << i << " type FAT32, starting lba: "
                  << first_block << " length: " << partition->block_count;

            fs = FAT32::create(device, { first_block, partition->block_count });

            break;
        }
        case ebr_partition_type: {
            log() << "Partition " << i << " type EBR, looking at chain...";

            auto region = MemoryManager::the().allocate_dma_buffer("EBR"_sv, Page::size);
            auto ebr_offset = partition->first_block + sector_offset;
            auto req = StorageDevice::AsyncRequest::make_read(region->virtual_range().begin(), { ebr_offset, 1 });
            device.submit_request(req);
            req.wait();

            load_mbr_partitions(device, region->virtual_range().begin(), ebr_offset);

            MemoryManager::the().free_virtual_region(*region);
            break;
        }
        default:
            log() << "Partition " << i << " has unknown type " << format::as_hex << partition->type << ", skipped";
        }

        if (!fs)
            continue;

        auto prefix = generate_prefix(device);
        m_prefix_to_fs[prefix] = fs;
        if (!m_prefix_to_fs.contains(boot_fs_prefix)) {
            log("VFS") << prefix.to_view() << " set as primary fs";
            m_prefix_to_fs[String(boot_fs_prefix)] = fs;
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

Pair<ErrorCode, RefPtr<FileDescription>> VFS::open(StringView path, FileDescription::Mode mode)
{
    auto split_path = split_prefix_and_path(path);

    if (!split_path)
        return { ErrorCode::BAD_PATH, RefPtr<FileDescription>() };

    auto& prefix_to_path = split_path.value();

    auto fs = m_prefix_to_fs.find(prefix_to_path.first);

    if (fs == m_prefix_to_fs.end())
        return { ErrorCode::DISK_NOT_FOUND, RefPtr<FileDescription>() };

    auto code_to_file = fs->second->open(prefix_to_path.second);
    if (code_to_file.first.is_error())
        return { code_to_file.first, RefPtr<FileDescription>() };

    return { ErrorCode::NO_ERROR, RefPtr<FileDescription>::create(*code_to_file.second, mode) };
}

ErrorCode VFS::close(FileDescription& fd)
{
    auto& file = fd.raw_file();
    auto code = file.fs().close(file);

    if (code.is_success())
        fd.mark_as_closed();

    return code;
}

ErrorCode VFS::remove(StringView path)
{
    auto split_path = split_prefix_and_path(path);

    if (!split_path)
        return ErrorCode::BAD_PATH;

    auto& prefix_to_path = split_path.value();

    auto fs = m_prefix_to_fs.find(prefix_to_path.first);

    if (fs == m_prefix_to_fs.end())
        return ErrorCode::DISK_NOT_FOUND;

    return fs->second->remove(prefix_to_path.second);
}

ErrorCode VFS::remove_directory(StringView path)
{
    auto split_path = split_prefix_and_path(path);

    if (!split_path)
        return ErrorCode::BAD_PATH;

    auto& prefix_to_path = split_path.value();

    auto fs = m_prefix_to_fs.find(prefix_to_path.first);

    if (fs == m_prefix_to_fs.end())
        return ErrorCode::DISK_NOT_FOUND;

    return fs->second->remove_directory(prefix_to_path.second);
}

ErrorCode VFS::create(StringView file_path, File::Attributes attributes)
{
    auto split_path = split_prefix_and_path(file_path);

    if (!split_path)
        return ErrorCode::BAD_PATH;

    auto& prefix_to_path = split_path.value();

    auto fs = m_prefix_to_fs.find(prefix_to_path.first);

    if (fs == m_prefix_to_fs.end())
        return ErrorCode::DISK_NOT_FOUND;

    return fs->second->create(prefix_to_path.second, attributes);
}

ErrorCode VFS::create_directory(StringView file_path, File::Attributes attributes)
{
    auto split_path = split_prefix_and_path(file_path);

    attributes |= File::Attributes::IS_DIRECTORY;

    if (!split_path)
        return ErrorCode::BAD_PATH;

    auto& prefix_to_path = split_path.value();

    auto fs = m_prefix_to_fs.find(prefix_to_path.first);

    if (fs == m_prefix_to_fs.end())
        return ErrorCode::DISK_NOT_FOUND;

    return fs->second->create_directory(prefix_to_path.second, attributes);
}

ErrorCode VFS::move(StringView path, StringView new_path)
{
    auto split_path_old = split_prefix_and_path(path);
    auto split_path_new = split_prefix_and_path(new_path);

    if (!split_path_old || !split_path_new)
        return ErrorCode::BAD_PATH;

    auto& prefix_to_path_old = split_path_old.value();
    auto& prefix_to_path_new = split_path_new.value();

    auto fs_1 = m_prefix_to_fs.find(prefix_to_path_new.first);
    auto fs_2 = m_prefix_to_fs.find(prefix_to_path_old.first);

    if (fs_1 == m_prefix_to_fs.end() || fs_2 == m_prefix_to_fs.end())
        return ErrorCode::DISK_NOT_FOUND;

    if (fs_1->second != fs_2->second) {
        warning() << "VFS: tried to move files cross FS, currently not implemented :(";
        return ErrorCode::UNSUPPORTED;
    }

    return fs_1->second->move(prefix_to_path_old.second, prefix_to_path_new.second);
}

void VFS::sync_all()
{
    for (auto& fs : m_prefix_to_fs) {
        if (fs.first == ""_sv)
            continue;

        fs.second->sync();
    }
}

void VFS::sync_thread()
{
    Thread::current()->set_invulnerable(true);

    static constexpr size_t sync_timeout_seconds = 30;

    for (;;) {
        sleep::for_seconds(sync_timeout_seconds);
        VFS::the().sync_all();
    }
}

}