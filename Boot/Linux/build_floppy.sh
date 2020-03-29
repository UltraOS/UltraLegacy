echo "Building UltraOS floppy image..."
MYDIR="$(dirname "$(realpath "$0")")"/..
mkdir -p $MYDIR/bin
nasm -i $MYDIR/Bootloader $MYDIR/Bootloader/UltraBoot.asm -o $MYDIR/bin/UltraBoot.bin
nasm -i $MYDIR/Bootloader $MYDIR/Bootloader/UKLoader.asm -o $MYDIR/bin/UKLoader.bin
mkdir -p $MYDIR/images
$MYDIR/Linux/ffc -b $MYDIR/bin/UltraBoot.bin -s $MYDIR/bin/UKLoader.bin -o $MYDIR/images/UltraFloppy.img --ls-fat
echo "Done!"

