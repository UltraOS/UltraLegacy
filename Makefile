images_dir = Images
iso_dir    = $(images_dir)/iso
kernel_dir = Kernel

# I don't like the hardcoded paths here
# TODO: find a way to make this more elegant
bootloader_dir = Boot/bin
bootloader_file = $(bootloader_dir)/UltraBoot.bin
kernelloader_file = $(bootloader_dir)/UKLoader.bin
kernel_file = $(kernel_dir)/Kernel.bin

dependencies = Boot/ \
               Kernel/

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

$(images_dir)/UltraFloppy.img: $(bootloader_file) $(kernelloader_file)
	mkdir -p $(images_dir)/
	Scripts/ffc -b $(bootloader_file) \
	            -s $(kernelloader_file) $(kernel_file)\
	            -o $(images_dir)/UltraFloppy.img \
	            --ls-fat

.PHONY: $(dependencies)
$(dependencies):
	$(MAKE) --no-print-directory -C $@

.PHONY: clean
clean:
	for dependency in $(dependencies); do \
		$(MAKE) -C $$dependency clean; \
	done
	rm -rf $(images_dir)/
