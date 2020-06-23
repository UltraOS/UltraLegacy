#!/bin/bash

on_error()
{
    echo "Failed to run clang format"
    exit 1
}

true_path="$(dirname "$(realpath "$0")")"
root_path=$true_path/..
path_to_kernel=$root_path/Kernel

find $path_to_kernel -regex ".*\.\(cpp\|h\)" -exec clang-format -style=file -i {} \; || on_error
