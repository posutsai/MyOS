; Reference
; 1. Print text with VGA video text mode: https://github.com/cirosantilli/x86-bare-metal-examples/tree/master/multiboot/hello-world
; 2. Register a keyboard handler: https://stackoverflow.com/questions/37618111/keyboard-irq-within-an-x86-kernel
global loader
global stack_ptr

; Macro to build a GDT descriptor entry
%define MAKE_GDT_DESC(base, limit, access, flags) \
    (((base & 0x00FFFFFF) << 16) | \
    ((base & 0xFF000000) << 32) | \
    (limit & 0x0000FFFF) | \
    ((limit & 0x000F0000) << 32) | \
    ((access & 0xFF) << 40) | \
    ((flags & 0x0F) << 52))

extern main

MODULEALIGN equ 1<<0
MEMINFO equ 1<<1
FLAGS equ MODULEALIGN | MEMINFO
MAGIC equ 0x1BADB002
CHECKSUM equ -(MAGIC + FLAGS)

section .mbheader
align 4
MultiBootHeader:
	dd MAGIC
	dd FLAGS
	dd CHECKSUM

section .text

STACKSIZE equ 0x4000

loader:
	mov esp, stack+STACKSIZE
	push eax
	push ebx
	lgdt [gdtr]
	jmp CODE32_SEL:.setcs
.setcs:
	mov ax, DATA32_SEL					; Setup the segment registers with our flat data selector
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	mov esp, stack_ptr				; set stack pointer
	call main

hang:
	hlt
	jmp hang

section .data
align 4
gdt_start:
    dq MAKE_GDT_DESC(0, 0, 0, 0); null descriptor
gdt32_code:
    dq MAKE_GDT_DESC(0, 0x00ffffff, 10011010b, 1100b) ; 32-bit code, 4kb gran, limit 0xffffffff bytes, base=0
gdt32_data:
    dq MAKE_GDT_DESC(0, 0x00ffffff, 10010010b, 1100b) ; 32-bit data, 4kb gran, limit 0xffffffff bytes, base=0
end_of_gdt:

gdtr:
    dw end_of_gdt - gdt_start - 1
                                ; limit (Size of GDT - 1)
    dd gdt_start                ; base of GDT
CODE32_SEL equ gdt32_code - gdt_start
DATA32_SEL equ gdt32_data - gdt_start

section .bss
align 4
stack:
	resb STACKSIZE
stack_ptr:
