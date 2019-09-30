#include "port_io.h"
#include "keyboard_map.h"
#include "utils.h"
#include "stdint.h"

extern void scheduler();

void kb_init(void)
{
	unsigned char curmask_master = read_port (0x21);
	write_port(0x21, curmask_master & 0xFC);
}

void keyboard_handler(void)
{
	signed char keycode;

	keycode = read_port(0x60);
	if(keycode >= 0 && keyboard_map[keycode]) {
		putc(keyboard_map[keycode]);
	}
	write_port(0x20, 0x20);
}

uint32_t time_frame = 0;
void timer_handler() {
	write_port(0x20, 0x20);
	if (time_frame == SWITCH_PERIOD) {
		// The interrupt guardiand here, scope between "cli" and "sti", is necessary because we
		// don't want the timer ISR is stopped by another timer interrupt.
		/* putc('*'); */
		cli();
		time_frame = 0;
		/* scheduler(); */
		sti();
	} else {
		time_frame++;
	}
}
