#include "stdint.h"
#ifndef _UTILS
#define _UTILS
#define COLS 80
#define ROWS 25
#define WHEEL_SPEED 50
#define SWITCH_PERIOD 5
#define sti() __asm__ ("sti\n\t")
#define cli() __asm__ ("cli\n\t")

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

extern void print_hex_uint32(uint32_t operand);
extern void putc(char c);
extern void puts(char *s);
extern uint16_t *const video;
extern uint16_t loc;
extern void roll_wheel(char n_task, enum color fg, enum color bg);
#endif
