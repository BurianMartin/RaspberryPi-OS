CXX := arm-none-eabi-g++
AS := arm-none-eabi-gcc
LD := arm-none-eabi-ld
OBJCOPY := arm-none-eabi-objcopy

MCPU := -mcpu=arm1176jzf-s -marm
CXXFLAGS := $(MCPU) -ffreestanding -fno-exceptions -fno-rtti -std=c++17 -Wall -Wextra -nostdlib -O2

.PHONY: hello_boot_test hello_boot_img clean
 
hello_boot_test: build/hello_boot.elf
	bash hello_boot/check_boot.sh
 
hello_boot_img: build/hello_boot.img

build/hello_boot.img: build/hello_boot.elf
	$(OBJCOPY) build/hello_boot.elf -O binary $@

build/hello_boot.elf: build/hello_boot/boot.o build/hello_boot/kernel.o hello_boot/linker.ld
	$(LD) -T hello_boot/linker.ld -o $@ build/hello_boot/boot.o build/hello_boot/kernel.o

build/hello_boot/boot.o: hello_boot/boot.s | build/hello_boot
	$(AS) $(MCPU) -c hello_boot/boot.s -o $@

build/hello_boot/kernel.o: hello_boot/kernel.cpp | build/hello_boot
	$(CXX) $(CXXFLAGS) -c hello_boot/kernel.cpp -o $@

build/hello_boot:
	mkdir -p build/hello_boot

clean:
	rm -rf build
