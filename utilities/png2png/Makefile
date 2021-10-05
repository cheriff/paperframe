COMPONENTS=../../components

CFLAGS=-O3 -Wall -Werror -I$(COMPONENTS)

all: png2png

miniz.o: $(COMPONENTS)/miniz/miniz.c
	$(CC) $(CFLAGS) -c -o $@ $^

stb_image.o: stb_image.h
	$(CC) -DSTB_IMAGE_IMPLEMENTATION=1 -xc stb_image.h -c -o stb_image.o

main.o: main.c
	$(CC) $(CFLAGS) -c -o $@ $^

png2png: main.o stb_image.o miniz.o
	$(CC) -O3 $^ -o $@

.PHONY: clean
clean:
	rm -f png2png main.o stb_image.o miniz.o
