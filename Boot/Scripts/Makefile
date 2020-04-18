# Rebuild the image if something changed
binary_files = ${BOOTLOADER_PATH}
images_folder = ${IMAGES_PATH}

image: $(images_folder)/UltraDisk.iso

$(images_folder)/UltraDisk.iso: $(images_folder)/UltraFloppy.img
	mkdir $(images_folder)/iso
	cp $(images_folder)/UltraFloppy.img $(images_folder)/iso/
	genisoimage -V 'UltraVolume' \
	            -input-charset iso8859-1 \
	            -o $(images_folder)/UltraDisk.iso \
	            -b UltraFloppy.img \
	            -hide UltraFloppy.img $(images_folder)/iso
	rm $(images_folder)/iso/UltraFloppy.img
	rmdir $(images_folder)/iso

$(images_folder)/UltraFloppy.img: $(binary_files)/UKLoader.bin $(binary_files)/UltraBoot.bin
	mkdir -p $(images_folder)
	./ffc -b $(binary_files)/UltraBoot.bin \
	      -s $(binary_files)/UKLoader.bin $(binary_files)/MyKernel.bin \
	      -o $(images_folder)/UltraFloppy.img \
	      --ls-fat

$(binary_files)/UKLoader.bin:

$(binary_files)/UltraBoot.bin:

.PHONY: clean
clean:
	rm $(images_folder)/UltraDisk.iso $(images_folder)/UltraFloppy.img
	rmdir $(images_folder)