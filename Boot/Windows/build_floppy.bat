@echo off
pushd %~dp0\..
echo Building UltraOS floppy image...
mkdir bin
nasm -i Bootloader Bootloader\UltraBoot.asm -o bin\UltraBoot.bin
nasm -i Bootloader Bootloader\UKLoader.asm -o bin\UKLoader.bin
mkdir images
Windows\ffc -b bin\UltraBoot.bin -s bin\UKLoader.bin -o images\UltraFloppy.img --ls-fat
echo Done!
popd