#include "FAT32.h"

namespace kernel {

RefPtr<FileSystem> FAT32::create(StorageDevice& associated_device, LBARange lba_range)
{
    auto* fat32 = new FAT32(associated_device, lba_range);
    if (fat32->try_initialize())
        return { static_cast<FileSystem*>(fat32) };

    return {};
}

FAT32::FAT32(StorageDevice& associated_device, LBARange lba_range)
    : FileSystem(associated_device, lba_range)
{
}

bool FAT32::try_initialize()
{
    static constexpr size_t ebpb_offset = 0x0B;

    auto ebpb_region = MemoryManager::the().allocate_dma_buffer("EBPB", ebpb_offset + EBPB::size);
    auto info = associated_device().query_info();
    auto lba_count = info.optimal_read_size / info.lba_size;
    ASSERT(lba_count != 0);

    auto request = StorageDevice::AsyncRequest::make_read(ebpb_region->virtual_range().begin(), { lba_range().begin(), lba_count });
    associated_device().submit_request(request);
    request.wait();

    copy_memory(ebpb_region->virtual_range().begin().as_pointer<u8>() + ebpb_offset, &m_ebpb, EBPB::size);

    MemoryManager::the().free_virtual_region(*ebpb_region);

    static constexpr u8 ebpb_signature = 0x29;
    static constexpr StringView fat32_filesystem = "FAT32   "_sv;

    if (m_ebpb.signature != ebpb_signature) {
        log("FAT32") << "bad EBPB signature, expected " << format::as_hex
                     << ebpb_signature << " got " << m_ebpb.signature;
        return false;
    }

    if (m_ebpb.filesystem_type != fat32_filesystem) {
        log("FAT32") << "unexpected filesystem type " << StringView(m_ebpb.filesystem_type, 8);
        return false;
    }

    log("FAT32") << "Successfully parsed EBPB: " << m_ebpb.fat_count << " FATs, "
                 << m_ebpb.sectors_per_cluster << " sector(s) per cluster, "
                 << m_ebpb.sectors_per_fat << " sectors per FAT";

    return true;
}

FAT32::File::File(StringView name, FileSystem& filesystem, Attributes attributes, u32 first_cluster)
    : BaseFile(name, filesystem, attributes)
    , m_first_cluster(first_cluster)
{
}

Pair<ErrorCode, FAT32::BaseFile*> FAT32::open(StringView)
{
    return { ErrorCode::UNSUPPORTED, nullptr };
}

ErrorCode FAT32::close(BaseFile&)
{
    return ErrorCode::UNSUPPORTED;
}

ErrorCode FAT32::remove(StringView)
{
    return ErrorCode::UNSUPPORTED;
}

ErrorCode FAT32::create(StringView, File::Attributes)
{
    return ErrorCode::UNSUPPORTED;
}

ErrorCode FAT32::move(StringView, StringView)
{
    return ErrorCode::UNSUPPORTED;
}

ErrorCode FAT32::copy(StringView, StringView)
{
    return ErrorCode::UNSUPPORTED;
}

}