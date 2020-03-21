echo "Building UltraOS iso image..."
MYDIR="$(dirname "$(realpath "$0")")"

/bin/bash $MYDIR/build_floppy.sh
genisoimage -V 'UltraVolume' \
            -input-charset iso8859-1 \
            -o $MYDIR/UltraDisk.iso \
            -b UltraFloppy.flp \
            -hide UltraFloppy.flp $MYDIR
echo "Done!"

