#include "stdint.h"
#include "string.h"

#define COLS 80
#define ROWS 25
#define FRAME_LEFT_TOP_X 30
#define FRAME_LEFT_TOP_Y 3
#define FRAME_WIDTH 25
#define FRAME_HEIGHT 20
#define BLANK_EL 0x

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

enum element {
	FRAME = 0,
	BLANK = 1,
	BRICK = 2
};


struct pg_element{
	enum color col: 4;
	enum element el: 4;
};

uint16_t *const video = (uint16_t*) 0xB8000;

void putc(uint8_t x, uint8_t y, enum color fg, enum color bg, char c) {
    video[y * COLS + x] = (bg << 12) | (fg << 8) | c;
}

void puts(uint8_t x, uint8_t y, enum color fg, enum color *bg, const char *s) {
    for (; *s; s++, x++)
        putc(x, y, fg, bg[x], *s);
}

void refresh_playground(struct pg_element *playground) {
	for (uint8_t y = 0; y < FRAME_HEIGHT; y++) {
		for (uint8_t x = 0; x < FRAME_WIDTH; x++) {
			struct pg_element v = playground[y * FRAME_WIDTH + x];
			switch(v.el) { // extract lowest 2 bit
				case FRAME:
					putc(x + FRAME_LEFT_TOP_X, y + FRAME_LEFT_TOP_Y, WHITE, WHITE, ' ');
					break;
				case BLANK:
					putc(x + FRAME_LEFT_TOP_X, y + FRAME_LEFT_TOP_Y, GRAY, BLACK, ':');
					break;
				case BRICK:
					putc(x + FRAME_LEFT_TOP_X, y + FRAME_LEFT_TOP_Y, WHITE, v.col & 0x0F, ' ');
					break;
			}
		}
	}
}

void init_playground(struct pg_element *playground) {
	uint8_t *p = (uint8_t *)playground;
	for (uint8_t row = 0; row < FRAME_HEIGHT - 1; row++, p += FRAME_WIDTH) {
		p[0] = 0x00;
		memset(p + 1, 0x10, FRAME_WIDTH - 2);
		*(p + FRAME_WIDTH - 1) = 0x00;
	}
	memset(p, 0x00, FRAME_WIDTH);
}

void clear(enum color bg) {
    uint8_t x, y;
    for (y = 0; y < ROWS; y++)
        for (x = 0; x < COLS; x++)
            putc(x, y, bg, bg, ' ');
}


int __attribute__((noreturn)) main() {
    clear(BLACK);
	struct pg_element playground[FRAME_WIDTH * FRAME_HEIGHT];
	init_playground(playground);
	refresh_playground(playground);
	char arr[16] = {65};
	memset(arr, 32, 16);
	enum color bg[16] = {0};
	for (int32_t i = 0; i < 16; i++)
		bg[i] = i;
    puts(0, 0, 1, bg, arr);
    while (1);
}
