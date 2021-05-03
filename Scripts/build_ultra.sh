#!/bin/bash

on_error()
{
    echo "Failed to build UltraOS!"
    exit 1
}

if [[ "$OSTYPE" == "darwin"* ]]; then
  realpath() {
      [[ $1 = /* ]] && echo "$1" || echo "$PWD/${1#./}"
  }

  cores=$(sysctl -n hw.physicalcpu)
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
  cores=$(nproc)
fi

echo "Building UltraOS..."
true_path="$(dirname "$(realpath "$0")")"
root_path=$true_path/..

arch="64"

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
cmake .. -DARCH=$arch || on_error
cmake --build . -- -j$cores || on_error
popd
popd

echo "Done!"
