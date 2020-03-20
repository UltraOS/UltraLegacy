echo "Building UltraOS floppy image..."
MYDIR="$(dirname "$(realpath "$0")")"
nasm $MYDIR/UltraBoot.asm -o $MYDIR/UltraBoot.bin
dd status=noxfer conv=notrunc if=$MYDIR/UltraBoot.bin of=$MYDIR/UltraFloppy.flp
echo "Done!"

