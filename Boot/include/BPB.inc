BPB:
    ; BIOS Parameter Block
    bytes_per_sector           dw 0
    sectors_per_cluster        db 0
    sectors_reserved           dw 0
    fat_table_count            db 0
    root_directory_entry_count dw 0
    unused_1                   dw 0
    media_descriptor           db 0
    unused_2                   dw 0
    sectors_per_track          dw 0
    head_count                 dw 0
    hidden_sector_count        dd 0
    total_sector_count         dd 0

    ; Extended BIOS Parameter Block
    sectors_per_fat_table      dd 0
    drive_description          dw 0
    version                    dw 0
    root_directory_cluster     dd 0
    fs_information             dw 0
    backup_boot_sector_count   dw 0
    reserved times 12          db 0
    boot_drive                 db 0
    unused_3                   db 0
    extended_boot_signature    db 0
    volume_id                  dd 0
    volume_label               db "UltraVolume"
    filesystem                 db "FAT32   "
