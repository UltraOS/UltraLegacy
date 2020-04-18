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

mkdir -p $root_path/bin || on_error

# Make the bootloader files
pushd $root_path/Bootloader
BOOTLOADER_PATH=$root_path/bin \
make || on_error

# a tempo hack
nasm MyKernel.asm -o  $root_path/bin/MyKernel.bin  || on_error
popd

# Make the images
pushd $true_path
IMAGES_PATH=$root_path/Images \
BOOTLOADER_PATH=$root_path/bin \
make || on_error
popd

echo "Done!"
