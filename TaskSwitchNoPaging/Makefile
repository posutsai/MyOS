.POSIX:
.PHONY: clean run run-img

main.img: main.elf
	cp '$<' iso/boot
	grub-mkrescue -o '$@' iso

# main.elf is the the multiboot file.
main.elf: entry.o main.o lowlevel.o isr.o utils.o
	ld -m elf_i386 -nostdlib -T linker.ld -o '$@' $^

entry.o: entry.asm
	nasm -f elf32 '$<' -o '$@'
lowlevel.o: lowlevel.asm
	nasm -f elf32 '$<' -o '$@'

main.o: main.c
	gcc -c -m32 -std=c99 -ffreestanding -fno-builtin -nostdlib --short-enums -Os -Iinclude -o '$@' -Wall -Wextra '$<'
isr.o: isr.c
	gcc -c -m32 -std=c99 -ffreestanding -fno-builtin -nostdlib --short-enums -Os -Iinclude -o '$@' -Wall -Wextra '$<'
utils.o: utils.c utils.h
	gcc -c -m32 -std=c99 -ffreestanding -fno-builtin -nostdlib --short-enums -Os -Iinclude -o '$@' -Wall -Wextra '$<'

clean:
	rm -f *.elf *.o iso/boot/*.elf *.img

run: main.elf
	qemu-system-i386 -kernel '$<'

run-img: main.img
	qemu-system-i386 -hda '$<'
