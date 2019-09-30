// Useful resource:
// 1. http://skelix.net/skelixos/tutorial06_en.html
#include "stdint.h"
#include "string.h"
#include "task.h"
#include "utils.h"

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

void keyboard_handler_int();
void timer_handler_int();
void load_idt(void*);
extern const uint32_t task1_run[];
extern const uint32_t task2_run[];

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
	struct Task task0 = {
		.tss = {
			.back_link = 0,
			.esp0 = &task0_stack0 + 256 * sizeof(uint32_t),
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
		},
		.tss_entry = 0,
		.ldt = {DEFAULT_LDT_CODE_SEG_DESCRIPTOR, DEFAULT_LDT_DATA_SEG_DESCRIPTOR},
		.ldt_entry = 0,
		.state = RUNNING,
		.priority = INITIAL_PRIORITY,
		.next = 0
	};
	return task0;
}

void do_task1() {
	uint16_t *const out = (uint16_t*) 0xB8000;
	puts("<<<< this is task1 >>>>");
	while(1);
}

void do_task2() {
	char *show = "Task2 !!!!";
	for (uint8_t i = 0; i < 10; i++) {
		puts(show);
	}
}

void clear() {
    uint8_t x, y;
    for (y = 0; y < ROWS; y++)
        for (x = 0; x < COLS; x++)
            putc(' ');
	loc = 0;
}

void new_task(struct Task *new_task, struct Task *task0, uint32_t eip, uint32_t stack0_addr, uint32_t stack3_addr) {
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

// Temporarily, we declare tasks as global variable for following several reasons. However
// it maybe issue in the future.
// 1. We need task0 as template while creating new task.
// 2. scheduler function in task.c need to access tasks.
struct Task scheduled_tasks[3];
struct Task *current_task;
int __attribute__((noreturn)) main() {
    clear(BLACK);
	current_task = scheduled_tasks;
	// Interrupt related operations
	idt_init();
	load_idt_entry(0x21, (unsigned long) keyboard_handler_int, 0x08, 0x8e);
	load_idt_entry(0x20, (unsigned long) timer_handler_int, 0x08, 0x8e);
	kb_init();
	scheduled_tasks[0] = get_task0();
	/* set_tss((uintptr_t) &(current_task->tss)); */
	print_hex_uint32(gdt32_tss[0]);
	print_hex_uint32(gdt32_tss[1]);
	uint64_t tss_entry = set_tss((uint32_t) &(current_task->tss));
	uint64_t ldt_entry = set_ldt((uint32_t) &(current_task->ldt));
	uint32_t *v = (uint32_t *)&tss_entry;
	gdt32_tss[0] = *v;
	gdt32_tss[1] = *(v+1);
	v = (uint32_t *)&ldt_entry;
	gdt32_tss[2] = *v;
	gdt32_tss[3] = *(v+1);
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
	new_task(scheduled_tasks+1, current_task,
		(uint32_t) &task1_run,
		(uint32_t) (&task1_stack0 + 1024 * sizeof(uint32_t)),
		(uint32_t) (&task1_stack3 + 1024 * sizeof(uint32_t))
	);
	new_task(&scheduled_tasks[2], current_task,
		(uint32_t) &task2_run,
		(uint32_t) (&task2_stack0 + 1024 * sizeof(uint32_t)),
		(uint32_t) (&task2_stack3 + 1024 * sizeof(uint32_t))
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
			"iret\n" \
			"1:\tmovw %%cx,%%ds\n\t" \
			"movw %%cx,%%es\n\t" \
			"movw %%cx,%%fs\n\t" \
			"movw %%cx,%%gs" \
			::"b"(LOCAL_CODE_SEL),"c"(LOCAL_DATA_SEL));
	roll_wheel('0', GRAY, BLACK);
	while(1);
}
