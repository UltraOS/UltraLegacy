#!/usr/bin/env python3

# KSyms.map file layout:
# [KSYMS] - a magic string indicating the beginning of KSyms file
# for each symbol
    # [size_t] - address of current symbol
    # [char[...]] - ascii representation of the symbol name
    # [0] - a null terminator for the ascii string
# [SMYSK] - a magic string indicating the end of KSyms file

import sys
import subprocess

if len(sys.argv) != 3:
    print(f"Usage: {sys.argv[0]} path/to/bin/dir name_of_elf_file")
    exit(1)

full_path = f"{sys.argv[1]}/{sys.argv[2]}"

result = subprocess.run(["nm", "-Cn", full_path], stdout=subprocess.PIPE)

if result.returncode != 0:
    print("nm returned with a non-zero error code")
    exit(1)

sym_string = result.stdout.decode("utf-8")

filtered_symbols = []

for symbol in sym_string.splitlines():
    as_list = symbol.split(" ")
    if as_list[1] not in ["T", "t", "W"]:
        continue

    while as_list[-1][-1] == "]":
        del as_list[-2:] # delete the [clone ...] part

    del as_list[1] # delete the symbol type

    filtered_symbols.append((int(as_list[0], 16), ' '.join(as_list[1:])))

sizeof_pointer = 4
if filtered_symbols[0][0] > 0xFFFFFFFF:
    sizeof_pointer = 8

with open(f"{sys.argv[1]}/KSyms.map", "wb") as f:
    f.write("KSYMS".encode())
    for symbol in filtered_symbols:
        f.write(symbol[0].to_bytes(sizeof_pointer, byteorder='little', signed=False))
        f.write(symbol[1].encode())
        f.write(int(0).to_bytes(1, byteorder='little'))
    f.write("SMYSK".encode())

