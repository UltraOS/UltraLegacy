images_dir = Images
iso_dir    = $(images_dir)/iso

# I don't like the hardcoded paths here
# TODO: find a way to make this more elegant
bootloader_dir = Boot/bin
bootloader_file = $(bootloader_dir)/UltraBoot.bin
bootloader_misc = $(bootloader_dir)/UKLoader.bin \
                  $(bootloader_dir)/MyKernel.bin

dependencies = Boot/

UltraOS: $(dependencies) images

images: $(images_dir)/UltraDisk.iso $(images_dir)/UltraFloppy.img

$(images_dir)/UltraDisk.iso: $(images_dir)/UltraFloppy.img
	mkdir -p $(iso_dir)
	cp $(images_dir)/UltraFloppy.img $(iso_dir)
	cd $(images_dir)/ && \
	genisoimage -V 'UltraVolume' \
	            -input-charset iso8859-1 \
	            -o UltraDisk.iso \
	            -b UltraFloppy.img \
	            -hide UltraFloppy.img iso
	rm $(iso_dir)/UltraFloppy.img
	rmdir $(iso_dir)

$(images_dir)/UltraFloppy.img: $(bootloader_file) $(bootloader_misc)
	mkdir -p $(images_dir)/
	Scripts/ffc -b $(bootloader_file) \
	            -s $(bootloader_misc) \
	            -o $(images_dir)/UltraFloppy.img \
	            --ls-fat

.PHONY: $(dependencies)
$(dependencies):
	$(MAKE) --no-print-directory -C $@

.PHONY: clean
clean:
	rm -rf $(images_dir)/