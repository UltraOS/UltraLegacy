BPB:
    ; BIOS Parameter Block
    BytesPerSector    dw 0
    SectorsPerCluster db 0
    SectorsReserved   dw 0
    FATCount          db 0
    RootDirEntries    dw 0
    Unused1           dw 0
    MediaDescriptor   db 0
    Unused2           dw 0
    SectorsPerTrack   dw 0
    Heads             dw 0
    SectorsHidden     dd 0
    TotalSectorCount  dd 0

    ; Extended BIOS Parameter Block
    SectorsPerFAT     dd 0
    DriveDescription  dw 0
    Version           dw 0
    RootDirCluster    dd 0
    FSInfomation      dw 0
    BackupBootSectors dw 0
    Reserved times 12 db 0
    BootDrive         db 0
    Unused3           db 0
    BootSignature     db 0
    VolumeID          dd 0
    VolumeLabel       db "UltraVolume"
    FileSystem        db "FAT32   "
