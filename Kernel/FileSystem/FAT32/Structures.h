#pragma once

#include "Common/Macros.h"
#include "Common/Types.h"

namespace kernel {
struct PACKED EBPB {
    // BPB
    u16 bytes_per_sector;
    u8 sectors_per_cluster;
    u16 reserved_sectors;
    u8 fat_count;
    u16 max_root_dir_entries;
    u16 unused_1; // total logical sectors for FAT12/16
    u8 media_descriptor;
    u16 unused_2; // logical sectors per file allocation table for FAT12/16
    u16 sectors_per_track;
    u16 heads;
    u32 hidden_sectors;
    u32 total_logical_sectors;

    // EBPB
    u32 sectors_per_fat;
    u16 drive_description;
    u16 version;
    u32 root_dir_cluster;
    u16 fs_information_sector;
    u16 backup_boot_sectors;
    u8 reserved[12];
    u8 drive_number;
    u8 unused_3;
    u8 signature;
    u32 volume_id;
    char volume_label[11];
    char filesystem_type[8];

    static constexpr size_t size = 79;
};

static_assert(sizeof(EBPB) == EBPB::size, "Incorrect size of EBPB");
}