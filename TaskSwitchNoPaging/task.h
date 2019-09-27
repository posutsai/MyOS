// Each TSS should maintain its CPL0 and CPL2 stack. Initially, we leave only
// stack0 for task0 and stack0 as well as stack3 for both task1 and task2. In
// this version, we easily simulate kernel and user space with GDT and LDT.

#define GLOBAL_CODE_SEL 0x08 // Code Segment selector offset in GDT
#define GLOBAL_DATA_SEL 0x10 // Data Segment selector offset in GDT
#define GLOBAL_TSS_SEL 0x18 // TSS selector offset in GDT
#define GLOBAL_LDT_SEL 0x20 // LDT selector offset in LDT

// Structure of selector in LDT
// 		15 ~ 3	   2 1 0
// +--------------+-+---+
// |	Index	  | |   |
// +--------------+-+---+
//
// Because 2nd bit is the identifier between GDT(0) and LDT(1), LDT selector
// starts from 0x04 whose RPL is 0. In our case RPL is required to be set as
// 3. Thus, LOCAL_CODE_SEL is 0x07 with 0 index. LOCAL_DATA_SEL is 0x0F with
// 1 index.

#define LOCAL_CODE_SEL 0x07 // Reference 0th entry in LDT selector with RPL3
#define LOCAL_DATA_SEL 0x0F // Reference 1th entry in LDT selector with RPL3

#define DEFAULT_LDT_CODE_SEG_DESCRIPTOR 0x00cffa000000ffffULL
#define DEFAULT_LDT_DATA_SEG_DESCRIPTOR 0x00cff2000000ffffULL

#define INITIAL_PRIORITY 200
const uint64_t *gdt_start;
extern void write_tss_descriptor(uint64_t);
extern void write_ldt_descriptor(uint64_t);

struct TaskStateSeg {
	uint32_t back_link;

	// upper 2 byte of ss is unused
	// sp -> stack pointer, ss -> stack segment
	uint32_t esp0, ss0;
	uint32_t esp1, ss1;
	uint32_t esp2, ss2;

	// Control Register 3 also known as Page Directory Base Register. We
	// don't use hardware paging feature in this version. Leave the register
	// empty temporarily.
	uint32_t cr3;

	// ip -> instruction pointer
	// ip and flags control current execution state.
	uint32_t eip;
	uint32_t eflags;

	// general register
	uint32_t eax, ecx, edx, ebx;
	uint32_t esp, ebp;
	uint32_t esi, edi;
	uint32_t es, cs, ss, ds, fs, gs;
	uint32_t ldt;
	uint32_t trace_bitmap;
};

enum TASK_STATE {
	RUNNING = 0,
	RUNABLE = 1,
	STOPPED = 2
};

// Form a task continuous linked list
struct Task {
	struct TaskStateSeg tss;
	uint64_t tss_entry;
	uint64_t ldt[2];
	uint64_t ldt_entry;
	enum TASK_STATE state;
	int priority;
	struct Task *next;
};

void set_tss(uint64_t tss_addr) {
	// Constant in  Task State Segment
	// limit 104 - 1 = 103 byte 0x67
	uint64_t tss_entry = 0x0080890000000067ULL; // initialize base as 0
	tss_entry |= ((tss_addr) << 16) & 0xffffff0000ULL;
	tss_entry |= ((tss_addr) << 32) & 0xff00000000000000ULL;
	write_tss_descriptor(tss_entry);
}

void set_ldt(uint64_t ldt_addr) {
	uint64_t ldt_entry = 0x008082000000000fULL;
	ldt_entry |= ((ldt_addr)<<16) & 0xffffff0000ULL;
	ldt_entry |= ((ldt_addr)<<32) & 0xff00000000000000ULL;
	write_ldt_descriptor(ldt_entry);
}

uint64_t get_tss_descriptor() {
	return gdt_start[3];
}

// This array space should be locate in .data section instead .bss section. The
// requirement of placing it into .bss is "initialization".
static uint32_t task0_stack0[256] = {0xF};

static uint32_t task1_stack0[1024] = {0xF};
static uint32_t task1_stack3[1024] = {0xF};

static uint32_t task2_stack0[1024] = {0xF};
static uint32_t task2_stack3[1024] = {0xF};

