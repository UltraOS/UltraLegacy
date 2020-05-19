cxx = i686-elf-g++
cxx_compile_flags = -std=c++17 -ffreestanding -O2 -Wall -Wextra -fno-use-cxa-atexit -fno-rtti -fno-exceptions
nasm_compile_flags = -felf32

ld_flags = -Wl,--oformat=binary -ffreestanding -O2 -nostdlib -lgcc

asm_files = $(wildcard ./*.asm)

crti_obj = crti.o
crtn_obj = crtn.o

object_files = Kernel.o crt0.o Itanium.o

crtbegin_obj = $(shell $(cxx) $(cxx_compile_flags) -print-file-name=crtbegin.o)
crtend_obj   = $(shell $(cxx) $(cxx_compile_flags) -print-file-name=crtend.o)

# Has to be in this order
obj_link_list = $(crti_obj) $(crtbegin_obj) $(object_files) $(crtend_obj) $(crtn_obj)

Kernel.bin: Linker.ld $(object_files) $(asm_files) $(crti_obj) $(crtn_obj)
	$(cxx) -T Linker.ld -o $@ $(ld_flags) $(obj_link_list)

%.o: %.cpp
	$(cxx) $(cxx_compile_flags) -c $< -o $@

%.o: %.asm
	nasm $(nasm_compile_flags) $< -o $@

.PHONY: clean
clean:
	rm -f $(object_files) $(crti_obj) $(crtn_obj) Kernel.bin