#!/bin/bash

on_error()
{
    echo "Failed to generate QTCreator project"
    exit 1
}

true_path="$(dirname "$(realpath "$0")")"
root_path=$true_path/..
path_to_kernel=$root_path/Kernel

project_name=ultra

source $true_path/utils.sh

pushd $path_to_kernel
project_files=$(find ./ -type f -name "*.cpp" -o -name "*.h" -o -name "*.asm")
project_directories=$(find ./ -type d)

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

printf "[General]\n" > $project_name.creator
printf "// Add predefined macros for your project here. For example:\n// #define THE_ANSWER 42\n#define ULTRA_$arch" > $project_name.config
printf "%s\n" $project_files > $project_name.files
printf "%s\n" $project_directories > $project_name.includes
printf "/std:c++17\n-std=c++17\n" > $project_name.cxxflags
popd
