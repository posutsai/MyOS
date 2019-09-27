// Useful resource:
// 1. http://skelix.net/skelixos/tutorial06_en.html
#include "stdint.h"
#include "string.h"
#include "task.h"

#define COLS 80
#define ROWS 25
#define FRAME_LEFT_TOP_X 30
#define FRAME_LEFT_TOP_Y 3
#define FRAME_WIDTH 25
#define FRAME_HEIGHT 20
#define BLANK_EL 0x
#include "port_io.h"

#define IDT_SIZE 256
#define PIC_1_CTRL 0x20
#define PIC_2_CTRL 0xA0
#define PIC_1_DATA 0x21
#define PIC_2_DATA 0xA1
#define sti() __asm__ ("sti\n\t")

void keyboard_handler_int();
void timer_handler_int();
void load_idt(void*);
const uintptr_t task1_run;
const uintptr_t task2_run;

struct idt_entry {
	unsigned short int offset_lowerbits;
	unsigned short int selector;
	unsigned char zero;
	unsigned char flags;
	unsigned short int offset_higherbits;
} __attribute__((packed));

struct idt_pointer {
	unsigned short limit;
	unsigned int base;
} __attribute__((packed));


struct idt_entry idt_table[IDT_SIZE];
struct idt_pointer idt_ptr;

void load_idt_entry(int isr_number, unsigned long base, short int selector, unsigned char flags) {
	idt_table[isr_number].offset_lowerbits = base & 0xFFFF;
	idt_table[isr_number].offset_higherbits = (base >> 16) & 0xFFFF;
	idt_table[isr_number].selector = selector;
	idt_table[isr_number].flags = flags;
	idt_table[isr_number].zero = 0;
}

static void initialize_idt_pointer() {
	idt_ptr.limit = (sizeof(struct idt_entry) * IDT_SIZE) - 1;
	idt_ptr.base = (unsigned int)&idt_table;
}

static void initialize_pic() {
	/* ICW1 - begin initialization */
	write_port(PIC_1_CTRL, 0x11);
	write_port(PIC_2_CTRL, 0x11);

	/* ICW2 - remap offset address of idt_table */
	/* In x86 protected mode, we have to remap the PICs beyond 0x20 because
	 * Intel have designated the first 32 interrupts as "reserved" for cpu exceptions */
	write_port(PIC_1_DATA, 0x20);
	write_port(PIC_2_DATA, 0x28);

	/* ICW3 - setup cascading */
	write_port(PIC_1_DATA, 0x00);
	write_port(PIC_2_DATA, 0x00);

	/* ICW4 - environment info */
	write_port(PIC_1_DATA, 0x01);
	write_port(PIC_2_DATA, 0x01);
	/* Initialization finished */

	/* mask interrupts */
	write_port(0x21 , 0xff);
	write_port(0xA1 , 0xff);
}

void idt_init() {
	initialize_pic();
	initialize_idt_pointer();
	load_idt(&idt_ptr);
}

struct Task get_task0() {
	struct TaskStateSeg task0_tss = {
		.back_link = 0,
		.esp0 = (uintptr_t)task0_stack0 + 256 * sizeof(uint32_t),
		.ss0 = GLOBAL_DATA_SEL,
		.esp1 = 0, .ss1 = 0, .esp2 = 0, .ss2 = 0,
		.cr3 = 0,
		.eip = 0, .eflags = 0,
		.eax = 0, .ecx = 0, .edx = 0, .ebx = 0,
		.esi = 0, .edi = 0,
		.es = LOCAL_DATA_SEL, .cs = LOCAL_CODE_SEL, .ds = LOCAL_DATA_SEL,
		.ss = LOCAL_DATA_SEL, .fs = LOCAL_DATA_SEL, .gs = LOCAL_DATA_SEL,
		.ldt = 0x20, // 4th entry of GDT
		.trace_bitmap = 0x00000000
	};
	struct Task task0 = {
		.tss = task0_tss,
		.tss_entry = 0,
		.ldt = {DEFAULT_LDT_CODE_SEG_DESCRIPTOR, DEFAULT_LDT_DATA_SEG_DESCRIPTOR},
		.ldt_entry = 0,
		.state = RUNNING,
		.priority = INITIAL_PRIORITY,
		.next = 0
	};
	return task0;
}

enum color {
    BLACK = 0,
	BLUE = 1,
	GREEN = 2,
	CYAN = 3,
	RED = 4,
	PURPLE = 5,
	BROWN = 6,
	GRAY = 7,
	DARK_GRAY = 8,
	LIGHT_BLUE = 9,
	LIGHT_GREEN = 10,
	LIGHT_CYAN = 11,
	LIGHT_RED = 12,
	LIGHT_PURPLE = 13,
	YELLOW = 14,
	WHITE = 15
};

uint16_t *const video = (uint16_t*) 0xB8000;
uint16_t loc = 0;

void print_hex_uint32(uint32_t operand) {
	for (uint8_t i = 8; i > 0; i--) {
		uint8_t B = operand >> ((i-1) << 2) & 0xF;
		uint8_t out = B > 9? B + 55: B + 48;
		video[loc++] = 0x07 << 8 | out;
	}
}

void putc(uint8_t x, uint8_t y, enum color fg, enum color bg, char c) {
    video[y * COLS + x] = (bg << 12) | (fg << 8) | c;
}

void puts(char *s) {
    for (; *s; s++)
       video[loc++] = 0x07 << 8 | *s;
}

void do_task1() {
	char *show = "Task1 !!!!";
	for (uint8_t i = 0; i < 10; i++, loc++) {
		if (loc == COLS * ROWS)
			loc = 0;
		video[loc] = 0x47 << 8 | show[i];
	}
}

void do_task2() {
	char *show = "Task2 !!!!";
	for (uint8_t i = 0; i < 10; i++, loc++) {
		if (loc == COLS * ROWS)
			loc = 0;
		video[loc] = 0x07 << 8 | show[i];
	}
}

void clear(enum color bg) {
    uint8_t x, y;
    for (y = 0; y < ROWS; y++)
        for (x = 0; x < COLS; x++)
            putc(x, y, bg, bg, ' ');
}

void new_task(struct Task *new_task, struct Task *task0, uintptr_t eip, uintptr_t stack0_addr, uintptr_t stack3_addr) {
	memcpy(new_task, task0, sizeof(struct Task));
	new_task->tss.esp0 = stack0_addr;
	new_task->tss.eip = eip;
	new_task->tss.eflags = 0x3202;
	new_task->tss.esp = stack3_addr;
	new_task->priority = INITIAL_PRIORITY;
	new_task->state = STOPPED;
	new_task->next = task0->next;
	task0->next = new_task;
	new_task->state = RUNABLE;
}

int __attribute__((noreturn)) main() {
    clear(BLACK);
	// Interrupt related operations
	idt_init();
	load_idt_entry(0x21, (unsigned long) keyboard_handler_int, 0x08, 0x8e);
	load_idt_entry(0x20, (unsigned long) timer_handler_int, 0x08, 0x8e);
	kb_init();
	char wheel[] = {'\\', '|', '/', '-'};
	int i = 0;

	struct Task task0 = get_task0();
	struct Task task1, task2;
	set_tss((uintptr_t) &(task0.tss));
	set_ldt((uintptr_t) &(task0.ldt));
	__asm__("ltrw %%ax\n\t"::"a"(GLOBAL_TSS_SEL));
	__asm__("lldt %%ax\n\t"::"a"(GLOBAL_LDT_SEL));

	// Reference how xv6 implement context switching
	// https://samwho.dev/blog/2013/06/10/context-switching-on-x86/
	// Although things may change in context switch implementation according to
	// virtual memory existance, there are many common steps between these two
	// scenarios. For example to establish the simplest scheduling strategy which
	// is round robin, timer plays an important role. We have to turn on the
	// interrupt handling before entering task0. Refer to "Disabling and
	// re-enabling interrupts" section in the webpage, both "cli" and "sti"
	// instructions affects the value in eflags and that's the reason why we have
	// to turn it on.
	sti();

	new_task(&task1, &task0,
		(uintptr_t)task1_run,
		(uintptr_t)task1_stack0 + 1024 * sizeof(uint32_t),
		(uintptr_t)task1_stack3 + 1024 * sizeof(uint32_t)
	);
	new_task(&task2, &task0,
		(uintptr_t)task2_run,
		(uintptr_t)task2_stack0 + 1024 * sizeof(uint32_t),
		(uintptr_t)task2_stack3 + 1024 * sizeof(uint32_t)
	);

	// Reference inline assembly follow AT&T syntex
	// https://wiki.osdev.org/Inline_Assembly
	//
	// "%%" represent "argument" from C code instead of normal registers. 'a' refers
	// to EAX, 'b' to EBX, 'c' to ECX, 'd' to EDX, 'S' to ESI, and 'D' to EDI. Two
	// consecutive colons means "no output" from assembly template.
	__asm__ ("movl %%esp,%%eax\n\t" \
			"pushl %%ecx\n\t" \
			"pushl %%eax\n\t" \
			"pushfl\n\t" \
			"pushl %%ebx\n\t" \
			"pushl $1f\n\t" \
			"1:\tmovw %%cx,%%ds\n\t" \
			"movw %%cx,%%es\n\t" \
			"movw %%cx,%%fs\n\t" \
			"movw %%cx,%%gs" \
			::"b"(LOCAL_CODE_SEL),"c"(LOCAL_DATA_SEL));
	// The 

	for (;;) {
		/* __asm__ ("movb    %%al,    0xb8000+160*24"::"a"(wheel[i])); */
		puts("hello");
		if (i == sizeof wheel)
			i = 0;
		else
			++i;
	}
	puts("hello");
	return 0;
}
