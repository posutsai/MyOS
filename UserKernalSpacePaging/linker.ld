ENTRY (loader)
/* We implement so-called "high half kernel" here. Briefly speadking, high half kernel
 * eliminate the gap between the memory model after turning paging system on. Since
 * paging system is implemented in hardware, CPU regard every address after activation
 * as virtual address. To really make use of virtual memory system - that is to say,
 * every process or task has independent memory space(4GB in 32bit), we place the
 * instructions of user space from the beginning of RAM. We divide the booting process
 * into three stages as following.
 *
 * Physical Memory						Virtual Memory
 * +--------+ 0x00000000				+-------+ 0x00000000
 * |		|							|		|
 * |		| kernel image-+			|		|
 * |		|			  	\			|		|
 * +--------+			 	 \			|		|
 * |		|				  \			|		|
 * |		|				   \		|		|
 * |		|					\		+-------+
 * |		|					 \		|		|
 * |		|					  \		|		| kernel image
 * |		|					   +-->	|		|
 * +--------+ 0xFFFFFFFF				+-------+ 0xFFFFFFFF
 *
 * 1. Before enabling paging system
 * The main purpose of such layout is to make user programs act as their memory address
 * start from 0x00000000. To do so, we place the kernel in higher half space. Let's say
 * 0xC0000000 or 3GB (it's just random number, as long as it is far enough from user
 * space). We assume the user programs uses less than 3GB. Here are several steps for
 * initializations
 *
 * 		a. The address in this file, linker script is in virtual memory domain. We tell
 * 		   the linker that start address of code section is 0xC0100000. The reason why it is
 * 		   not exactly 0xC0000000 is that Grub require at least 1Mb.
 *
 * 		b. Page table initialization, I'll leave the implementation detail in entry.asm
 * */
SECTIONS
{
  . = 0xC0100000; /* 3Gb + 1 Mb*/
  _kernel_start = .;
  .text ALIGN(0x1000) : AT(ADDR(.text) - 0xC0000000) {
	*(.mbheader)
    *(.text)
  }
  .rodata ALIGN (0x1000) : AT(ADDR(.rodata) - 0xC0000000){
    *(.rodata)
  }
  .data ALIGN (0x1000) : AT(ADDR(.data) - 0xC0000000){
    *(.data)
  }
  .bss ALIGN (0x1000) : AT(ADDR(.bss) - 0xC0000000){
    sbss = .;
    *(COMMON)
    *(.bss)
    ebss = .;
  }
  _kernel_end = .;
}
