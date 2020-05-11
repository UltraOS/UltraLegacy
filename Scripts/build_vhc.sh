#!/bin/bash

true_path="$(dirname "$(realpath "$0")")"

on_error()
{
    echo "VHC build failed!"
    exit 1
}

pushd () {
    command pushd "$@" > /dev/null
}

popd () {
    command popd "$@" > /dev/null
}

pushd $true_path

if [ -e "./vhc" ]
then
  echo "VHC is already built!"
  exit 0
else
  echo "Building VHC..."
fi

git clone https://github.com/UltraOS/VHC vhc-src || on_error
pushd vhc-src
mkdir build || on_error
pushd build
cmake .. && cmake --build . || on_error
mv ./vhc $true_path || on_error
popd
popd
rm -rf vhc-src || on_error

popd

echo "Successfully built VHC"