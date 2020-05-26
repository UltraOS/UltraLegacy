cxx = i686-elf-g++
cxx_compile_flags = -std=c++17 -ffreestanding -O2 -Wall -Wextra -fno-use-cxa-atexit -fno-rtti -fno-exceptions
nasm_compile_flags = -felf32

ld_flags = -Wl,--oformat=binary -ffreestanding -O2 -nostdlib -lgcc

asm_files = $(wildcard ./*.asm)

object_files = Kernel.o crt0.o Itanium.o InterruptServiceRoutines.o InterruptDescriptorTable.o


Kernel.bin: Linker.ld $(object_files) $(asm_files) $(crti_obj) $(crtn_obj)
	$(cxx) -T Linker.ld -o $@ $(ld_flags) $(object_files)

%.o: %.cpp
	$(cxx) $(cxx_compile_flags) -c $< -o $@

%.o: %.asm
	nasm $(nasm_compile_flags) $< -o $@

.PHONY: clean
clean:
	rm -f $(object_files) $(crti_obj) $(crtn_obj) Kernel.bin