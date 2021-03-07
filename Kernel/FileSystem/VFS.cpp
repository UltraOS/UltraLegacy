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
    auto lba0_region = MemoryManager::the().allocate_kernel_private_anywhere("LBA0"_sv, Page::size);
    auto& lba0 = *static_cast<PrivateVirtualRegion*>(lba0_region.get());
    lba0.preallocate_range();

    auto info = device.query_info();
    auto lba_count = Page::size / info.lba_size;
    ASSERT(lba_count != 0); // just in case lba size is somehow greater than page size

    device.read_synchronous(lba0.virtual_range().begin(), { 0, lba_count });

    static constexpr StringView gpt_signature = "EFI PART"_sv;
    static constexpr size_t offset_to_gpt_signature = 512;
    auto lba0_data = lba0.virtual_range().begin().as_pointer<char>();

    if (StringView(lba0_data + offset_to_gpt_signature, gpt_signature.size()) == gpt_signature) {
        warning() << "VFS: detected a GPT-partitioned device " << device.device_model()
                  << " which is currently not supported";
        return;
    }

    static constexpr u16 mbr_partition_signature = 0xAA55;
    static constexpr size_t offset_to_mbr_signature = 510;
    auto signature = *Address(lba0_data + offset_to_mbr_signature).as_pointer<u16>();
    if (signature != mbr_partition_signature) {
        warning() << "VFS: device " << device.device_model() << " contains an invalid MBR signature "
                  << format::as_hex << "(expected: " << mbr_partition_signature << " got: " << signature;
        return;
    }

    log() << "VFS: detected an MBR formatted storage device " << device.device_model();
    load_mbr_partitions(device, lba0.virtual_range().begin());
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

    String prefix = info.medium_type_to_string();
    prefix += allocate_prefix(info.medium_type);

    log("VFS") << "allocated new prefix " << prefix.to_view();

    return prefix;
}

}