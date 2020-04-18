out_path = ${BOOTLOADER_PATH}

bootloader: $(out_path)/UltraBoot.bin $(out_path)/UKLoader.bin

$(out_path)/UltraBoot.bin: UltraBoot.asm
	nasm UltraBoot.asm -o $(out_path)/UltraBoot.bin

$(out_path)/UKLoader.bin: UKLoader.asm
	nasm UKLoader.asm -o $(out_path)/UKLoader.bin

.PHONY: clean
clean:
	rm $(out_path)/UKLoader.bin $(out_path)/UltraBoot.bin