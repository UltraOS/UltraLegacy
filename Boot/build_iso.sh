echo "Building UltraOS iso image..."
MYDIR="$(dirname "$(realpath "$0")")"

/bin/bash $MYDIR/build_floppy.sh
mkdir $MYDIR/iso
cp $MYDIR/UltraFloppy.flp $MYDIR/iso/
genisoimage -V 'UltraVolume' \
            -input-charset iso8859-1 \
            -o $MYDIR/UltraDisk.iso \
            -b UltraFloppy.flp \
            -hide UltraFloppy.flp $MYDIR/iso
rm $MYDIR/iso/UltraFloppy.flp
rmdir $MYDIR/iso
echo "Done!"

