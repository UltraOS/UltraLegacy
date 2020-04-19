%ifndef _BPB_INC_
%define _BPB_INC_

BPB:
    ; BIOS Parameter Block
    OEMLabel          db "UltraOS "
    BytesPerSector    dw 512
    SectorsPerCluster db 1
    SectorsReserved   dw 1
    FATCount          db 2
    RootDirEntries    dw 224
    SectorsCount      dw 2880
    MediaDescriptor   db 0xF0 ; gonna pretend we're a 3.5 floppy
    FATSize           dw 9
    SectorsPerTrack   dw 18
    NumberOfSides     dw 2
    SectorsHidden     dd 0
    SectorsLarge      dd 0

    ; Extended BIOS Parameter Block
    BootDrive         db 0
    Reserved          db 0
    BootSignature     db 0x29
    VolumeID          dd 0
    VolumeLabel       db "UltraVolume"
    FileSystem        db "FAT12   "

%endif ; _BPB_INC_
