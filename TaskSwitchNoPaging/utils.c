#include "stdint.h"
#include "utils.h"
uint16_t *const video = (uint16_t*) 0xB8000;
uint16_t loc = 0;
void print_hex_uint32(uint32_t operand) {
	for (uint8_t i = 8; i > 0; i--) {
		uint8_t B = operand >> ((i-1) << 2) & 0xF;
		uint8_t out = B > 9? B + 55: B + 48;
		putc(out);
	}
}

void putc(char c) {
    video[loc++] = 0x07 << 8 | c;
}

void puts(char *s) {
    for (; *s; s++)
       video[loc++] = 0x07 << 8 | *s;
}

void puts_color(uint8_t x, uint8_t y, enum color fg, enum color bg, const char *s) {
	int pos = y * COLS + x;
	for (; *s; s++, pos++)
		video[pos] = (bg << 12) | (fg << 8) | *s;
}

void roll_wheel(char n_task, enum color fg, enum color bg) {
	char wheel[] = {'\\', '|', '/', '-'};
	char *show = "task   ";
	show[4] = n_task;
	int i = 0;
	for (;;) {
		if (!(i % WHEEL_SPEED)) {
			show[6] = wheel[i/WHEEL_SPEED];
			puts_color(0, 1, fg, bg, show);
		}
		if (i == sizeof(char) * WHEEL_SPEED)
			i = 0;
		else
			++i;
	}

}
