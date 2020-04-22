gpp = i686-elf-g++
gpp_compile_flags = -std=c++17 -ffreestanding -O2 -Wall -Wextra
nasm_compile_flags = -felf32

ld_flags = -Wl,--oformat=binary -ffreestanding -O2 -nostdlib -lgcc

object_files = Kernel.o Entrypoint.o

Kernel.bin: Linker.ld $(object_files)
	$(gpp) -T Linker.ld -o $@ $(ld_flags) $(object_files)

%.o: %.cpp
	$(gpp) $(gpp_compile_flags) -c $< -o $@

%.o: %.asm
	nasm $(nasm_compile_flags) $< -o $@

.PHONY: clean
clean:
	rm $(object_files) Kernel.bin