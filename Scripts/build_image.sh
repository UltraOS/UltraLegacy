on_error()
{
    echo "Failed to build the image!"
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
make || on_error
popd

echo "Done!"
