#!/bin/bash

if [[ "$OSTYPE" == "darwin"* ]]; then
  realpath() {
      [[ $1 = /* ]] && echo "$1" || echo "$PWD/${1#./}"
  }
fi

on_error()
{
    echo "Failed to run clang format"
    exit 1
}

true_path="$(dirname "$(realpath "$0")")"
root_path=$true_path/..
path_to_kernel=$root_path/Kernel

find $path_to_kernel -type f \( -name "*cpp" -o -name "*h" \) -exec clang-format -style=file -i {} \; || on_error
