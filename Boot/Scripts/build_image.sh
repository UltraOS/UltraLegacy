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
nasm  UltraBoot.asm -o $root_path/bin/UltraBoot.bin || on_error
nasm  UKLoader.asm -o  $root_path/bin/UKLoader.bin  || on_error
nasm  MyKernel.asm -o  $root_path/bin/MyKernel.bin  || on_error
popd

mkdir -p images || on_error
$true_path/ffc -b bin/UltraBoot.bin \
      -s bin/UKLoader.bin bin/MyKernel.bin \
      -o images/UltraFloppy.img \
      --ls-fat || on_error

mkdir images/iso || on_error
cp images/UltraFloppy.img images/iso/
genisoimage -V 'UltraVolume' \
            -input-charset iso8859-1 \
            -o images/UltraDisk.iso \
            -b UltraFloppy.img \
            -hide UltraFloppy.img images/iso || on_error

rm images/iso/UltraFloppy.img
rmdir images/iso
popd

echo "Done!"

