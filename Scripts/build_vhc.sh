#!/bin/bash

true_path="$(dirname "$(realpath "$0")")"

source $true_path/utils.sh

on_error()
{
    echo "VHC build failed!"
    rm -rf vhc-src
    exit 1
}

pushd $true_path

if [ -e "./vhc" ]
then
  exit 0
else
  echo "Building VHC..."
fi

git clone https://github.com/UltraOS/VHC vhc-src || on_error
pushd vhc-src
mkdir build || on_error
pushd build
cmake .. -DCMAKE_BUILD_TYPE=Release && cmake --build . || on_error
mv ./vhc $true_path || on_error
popd
popd
rm -rf vhc-src || on_error

popd

echo "Successfully built VHC"