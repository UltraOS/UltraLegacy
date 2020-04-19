bin_dir       = bin/
include_dir   = include
include_files = $(wildcard $(include_dir)/*.inc)
src_files     = $(wildcard ./*.asm)
bin_files     = $(patsubst %.asm, $(bin_dir)%.bin, $(notdir $(src_files)))

bootloader: $(bin_files)

$(bin_dir)%.bin: $(src_dir)%.asm  $(include_files) | $(bin_dir)
	nasm -I $(include_dir)/ $< -o $@

$(bin_dir):
	mkdir -p $(bin_dir)

.PHONY: clean
clean:
	rm -rf $(bin_dir)