echo "Building UltraOS iso image..."
MYDIR="$(dirname "$(realpath "$0")")"

/bin/bash $MYDIR/build_floppy.sh
mkdir $MYDIR/../images/iso
cp $MYDIR/../images/UltraFloppy.img $MYDIR/../images/iso/
genisoimage -V 'UltraVolume' \
            -input-charset iso8859-1 \
            -o $MYDIR/../images/UltraDisk.iso \
            -b UltraFloppy.img \
            -hide UltraFloppy.img $MYDIR/../images/iso
rm $MYDIR/../images/iso/UltraFloppy.img
rmdir $MYDIR/../images/iso
echo "Done!"

