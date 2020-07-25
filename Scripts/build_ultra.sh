#!/bin/bash

on_error()
{
    echo "Failed to build UltraOS!"
    exit 1
}

echo "Building UltraOS..."
true_path="$(dirname "$(realpath "$0")")"
root_path=$true_path/..

arch="32"

if [ "$1" ]
  then
    if [ $1 != "64" ] && [ $1 != "32" ]
    then
      echo "Unknown architecture $1"
      on_error
    else
      arch="$1"
    fi
fi

source $true_path/utils.sh

pushd $root_path
mkdir -p Build$arch || on_error
pushd Build$arch
cmake .. -DARCH=$1 || on_error
cmake --build . || on_error
popd
popd

echo "Done!"
