all: build run

build:
	nasm main.asm -f bin -o main.bin
	truncate -s 1440k main.bin

run:
	qemu-system-i386 -fda main.bin

clean:
	rm -f main.bin
