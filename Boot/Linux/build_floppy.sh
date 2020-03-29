echo "Building UltraOS floppy image..."
MYDIR="$(dirname "$(realpath "$0")")"
nasm $MYDIR/UltraBoot.asm -o $MYDIR/UltraBoot.bin
dd if=/dev/zero of=$MYDIR/UltraFloppy.flp bs=1024 count=1440
dd if=$MYDIR/UltraBoot.bin of=$MYDIR/UltraFloppy.flp seek=0 count=1 conv=notrunc
echo "Done!"

