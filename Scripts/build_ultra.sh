#!/bin/bash

on_error()
{
    echo "Failed to build UltraOS!"
    exit 1
}

echo "Building UltraOS..."
true_path="$(dirname "$(realpath "$0")")"
root_path=$true_path/..

source $true_path/utils.sh

pushd $root_path
mkdir -p Build || on_error
pushd Build
cmake .. || on_error
cmake --build . || on_error
popd
popd

echo "Done!"
