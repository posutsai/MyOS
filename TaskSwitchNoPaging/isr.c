#include "port_io.h"
#include "keyboard_map.h"
#include "utils.h"

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

void timer_handler() {
	/* vidptr[current_loc++] = '.'; */
	/* vidptr[current_loc++] = 0x07; */
	write_port(0x20, 0x20);
}
