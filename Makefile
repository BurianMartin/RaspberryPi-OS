CXX := arm-none-eabi-g++
AS := arm-none-eabi-gcc
LD := arm-none-eabi-ld
OBJCOPY := arm-none-eabi-objcopy

MCPU := -mcpu=arm1176jzf-s -marm
INCLUDES := -Ikernel/include
CXXFLAGS := $(MCPU) -ffreestanding -fno-exceptions -fno-rtti -std=c++17 -Wall -Wextra -nostdlib -O2 $(INCLUDES)

.PHONY: kernel_test kernel_img clean

kernel_test: build/kernel.elf
	bash check_boot.sh

kernel_img: build/kernel.img

build/kernel.img: build/kernel.elf
	$(OBJCOPY) build/kernel.elf -O binary $@

build/kernel.elf: build/boot.o build/context.o build/context_cpp.o build/uart.o build/utils.o build/kernel.o linker.ld
	$(LD) -T linker.ld -o $@ build/boot.o build/context.o build/context_cpp.o build/uart.o build/utils.o build/kernel.o

build/boot.o: kernel/asm/boot.s | build
	$(AS) $(MCPU) -c kernel/asm/boot.s -o $@

build/context.o: kernel/asm/context.s | build
	$(AS) $(MCPU) -c kernel/asm/context.s -o $@

build/context_cpp.o: kernel/src/context.cpp kernel/include/context.hpp | build
	$(CXX) $(CXXFLAGS) -c kernel/src/context.cpp -o $@

build/uart.o: kernel/src/uart.cpp kernel/include/uart.hpp kernel/include/peripherals.hpp | build
	$(CXX) $(CXXFLAGS) -c kernel/src/uart.cpp -o $@

build/utils.o: kernel/src/utils.cpp kernel/include/utils.hpp kernel/include/peripherals.hpp | build
	$(CXX) $(CXXFLAGS) -c kernel/src/utils.cpp -o $@

build/kernel.o: kernel/kernel.cpp kernel/include/uart.hpp kernel/include/utils.hpp kernel/include/peripherals.hpp | build
	$(CXX) $(CXXFLAGS) -c kernel/kernel.cpp -o $@

build:
	mkdir -p build

clean:
	rm -rf build
