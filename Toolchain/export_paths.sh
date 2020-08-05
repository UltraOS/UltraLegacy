#!/bin/bash

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

true_path="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PATH="$true_path/CrossCompiler/Tools$arch/bin:$PATH"
