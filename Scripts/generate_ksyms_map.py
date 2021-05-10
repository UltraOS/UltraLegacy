#!/usr/bin/env python3

# KSyms.map file layout:
# [KSYMS] - a magic string indicating the beginning of KSyms file
# for each symbol
    # [size_t] - address of current symbol
    # [char[MAX_SYMBOL_LENGTH]] - ascii representation of the symbol name, terminated with null
# [SMYSK] - a magic string indicating the end of KSyms file

import sys
import subprocess
from platform import uname

if len(sys.argv) < 3 or len(sys.argv) > 4:
    print(f"Usage: {sys.argv[0]} path/to/bin/dir name_of_kernel_map <path to c++filt>")
    exit(1)

MAX_SYMBOL_LENGTH = 100
full_path = f"{sys.argv[1]}/{sys.argv[2]}"
symbols = []
demangle_command = "c++filt" if len(sys.argv) < 4 else sys.argv[3]

# If we're inside WSL running c++filt from a mounted system volume would be incredibly slow,
# so revert to using the native toolchain c++filt (which is hopefully GCC).
if 'microsoft' in uname().release.lower():
    demangle_command = "c++filt"

with open(full_path, "r") as f:
    previous_address = ""

    while True:
        line = f.readline()
        if not line:
            break

        as_list = line.split(" ")
        as_list = [string for string in as_list if string]

        if len(as_list) == 2 and as_list[0].startswith("0x"):
            if previous_address == as_list[0]:
                continue

            previous_address = as_list[0]

            symbol = as_list[1].rstrip('\n')

            result = subprocess.run([demangle_command, symbol], stdout=subprocess.PIPE)
            if result.returncode != 0:
                exit(1)
                print("c++filt returned with a non-zero exit code")

            demangled_symbol = result.stdout.decode('utf-8').replace('kernel::', '').rstrip('\n')

            if demangled_symbol.startswith("vtable for"):
                continue

            if "::s_" in demangled_symbol:
                continue

            if demangled_symbol.startswith("interrupt") and demangled_symbol.endswith("_handler"):
                continue

            symbol_length = len(demangled_symbol)
            if symbol_length > MAX_SYMBOL_LENGTH - 1:
                continue

            symbols.append((int(as_list[0], 16), demangled_symbol + ("\0" * (MAX_SYMBOL_LENGTH - symbol_length - 1))))

sizeof_pointer = 4
if symbols[0][0] > 0xFFFFFFFF:
    sizeof_pointer = 8

with open(f"{sys.argv[1]}/KSyms.map", "wb") as f:
    f.write("KSYMS".encode())
    for symbol in symbols:
        f.write(symbol[0].to_bytes(sizeof_pointer, byteorder='little', signed=False))
        f.write(symbol[1].encode())
        f.write(int(0).to_bytes(1, byteorder='little'))
    f.write("SMYSK".encode())
