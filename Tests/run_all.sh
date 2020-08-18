on_error()
{
    echo "Failed to run tests!"
    exit 1
}

mkdir -p build  || on_error
pushd build
cmake ..        || on_error
cmake --build . || on_error
popd
clear
./bin/RunTests