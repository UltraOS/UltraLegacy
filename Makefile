images_dir = Images
kernel_dir = Kernel

# I don't like the hardcoded paths here
# TODO: find a way to make this more elegant
bootloader_dir = Boot/bin
mbr_file = $(bootloader_dir)/UltraMBR.bin
vbr_file = $(bootloader_dir)/UltraBoot.bin
kernelloader_file = $(bootloader_dir)/UKLoader.bin
kernel_file = $(kernel_dir)/Kernel.bin

dependencies = Boot/ \
               Kernel/

.PHONY: UltraOS
UltraOS: $(dependencies) $(images_dir)/UltraHDD.vmdk

$(images_dir)/UltraHDD.vmdk: $(mbr_file) $(vbr_file) $(kernelloader_file) $(kernel_file)
	mkdir -p $(images_dir)/
	Scripts/vhc --mbr $(mbr_file) \
	            --vbr $(vbr_file) \
	            --files $(kernelloader_file) $(kernel_file) \
	            --size 64 --image-dir $(images_dir) --image-name UltraHDD

.PHONY: $(dependencies)
$(dependencies):
	$(MAKE) --no-print-directory -C $@

.PHONY: clean
clean:
	for dependency in $(dependencies); do \
		$(MAKE) -C $$dependency clean; \
	done
	rm -rf $(images_dir)/
