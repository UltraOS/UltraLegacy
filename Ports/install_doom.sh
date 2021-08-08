#!/bin/bash

on_error()
{
    echo "Failed to install DOOM!"
    exit 1
}

true_path="$(dirname "$(realpath "$0")")"

source $true_path/../Scripts/utils.sh

pushd $true_path
git clone https://github.com/UltraOS/doomgeneric UltraDOOM || on_error
touch ports_list.txt
echo "add_subdirectory(UltraDOOM)" >> ports_list.txt

echo \
"============================================================================================
Successfully downloaded the UltraOS DOOM port.
Don't forget to add DOOM1.WAD (must be named exactly that) to $true_path/UltraDOOM,
as it's mandatory to be able to run DOOM since it contains all assets like maps and textures.
You can acquire it buy buying DOOM from GOG at https://www.gog.com/game/the_ultimate_doom :)
============================================================================================"

popd $true_pathg
