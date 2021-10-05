PICOTOOL=picotool/picotool

SRC_FILES:=$(wildcard *c *.h) CMakeLists.txt

all: build/paperframe.elf pngpush

pico-sdk:
	git clone https://github.com/raspberrypi/pico-sdk
	( cd pico-sdk && git submodule update --init )

$(PICOTOOL): | pico-sdk
	git clone https://github.com/raspberrypi/picotool.git
	(cd picotool && cmake . -DPICO_SDK_PATH=../pico-sdk )
	make -C picotool

build: | pico-sdk
	mkdir build
	( cd build && cmake ../ )

build/paperframe.elf: $(SRC_FILES) | build
	$(MAKE) -C build

.PHONY: run
run: build/paperframe.elf | $(PICOTOOL)
	$(PICOTOOL) load -x $<

.PHONY: clean
clean:
	rm -rf build


# write a file to flash, becomes readable at runtime.
# note: the load address is near the end of flash, for large files we may have
#       to bring it forward. But this then limits the size of the executable ..
#FLASH_FILE=somefile.png
#FLASH_ADDR=0x101e2000  # This gives max filesize of 120KB
#.PHONY: flash
#flash: | $(PICOTOOL)
#	$(PICOTOOL) load -v $(FLASH_FILE) -t bin -o $(FLASH_ADDR)


pngpush: ../utilities/pngpush.c
	$(CC) -O3 -Wall -Werror -o $@ $<
