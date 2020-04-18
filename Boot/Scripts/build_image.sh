on_error()
{
    echo "Failed to build the floppy image!"
    exit 1
}

pushd () {
    command pushd "$@" > /dev/null
}

popd () {
    command popd "$@" > /dev/null
}

echo "Building UltraOS image..."
true_path="$(dirname "$(realpath "$0")")"
root_path=$true_path/..

pushd $root_path
mkdir -p bin || on_error

pushd $root_path/Bootloader
BOOTLOADER_PATH=$root_path/bin make || on_error
nasm  MyKernel.asm -o  $root_path/bin/MyKernel.bin  || on_error
popd

mkdir -p Images || on_error
$true_path/ffc -b bin/UltraBoot.bin \
               -s bin/UKLoader.bin bin/MyKernel.bin \
               -o Images/UltraFloppy.img \
               --ls-fat || on_error

mkdir Images/iso || on_error
cp Images/UltraFloppy.img Images/iso/
genisoimage -V 'UltraVolume' \
            -input-charset iso8859-1 \
            -o Images/UltraDisk.iso \
            -b UltraFloppy.img \
            -hide UltraFloppy.img Images/iso || on_error

rm Images/iso/UltraFloppy.img
rmdir Images/iso
popd

echo "Done!"

