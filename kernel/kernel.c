#include "../drivers/video.h"
#include "../drivers/sysclock.h"
#include "../drivers/kbrd.h"
#include "../drivers/rs232.h"
#include "../drivers/floppy.h"
#include "../drivers/ramdisk.h"
#include "../drivers/isa_dma.h"

#include "interrupt.h"
#include "memorymanager.h"
#include "multiboot.h"
#include "task.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>
#include <elf.h>

#include "multiboot.h"
extern multiboot_info* _multiboot;

extern void _IMAGE_END_;
extern void _BSS_END_;
extern void _DATA_END_;

void load_floppy_driver()
{
	dynamic_object ob;
	ob.lib_set = hashmap_create(16);
	ob.symbol_map = hashmap_create(16);
	ob.glob_data_symbol_map = hashmap_create(16);

	hashmap_insert(ob.symbol_map, "irq_install_handler", (uintptr_t)&irq_install_handler);
	hashmap_insert(ob.symbol_map, "printf", (uintptr_t)&printf);
	hashmap_insert(ob.symbol_map, "filesystem_add_drive", (uintptr_t)&filesystem_add_drive);
	hashmap_insert(ob.symbol_map, "sysclock_sleep", (uintptr_t)&sysclock_sleep);
	hashmap_insert(ob.symbol_map, "isa_dma_begin_transfer", (uintptr_t)&isa_dma_begin_transfer);

	load_elf("floppy.drv", &ob, false);

	uint32_t func_address;
	if(hashmap_lookup(ob.symbol_map, "floppy_init", &func_address))
	{
		((void (*)())func_address)();
	}
	else
	{
		printf("cannot find function\n");
	}
}

void load_kbrd_driver()
{
	dynamic_object ob;
	ob.lib_set = hashmap_create(16);
	ob.symbol_map = hashmap_create(16);
	ob.glob_data_symbol_map = hashmap_create(16);

	hashmap_insert(ob.symbol_map, "handle_keyevent", (uintptr_t)&handle_keyevent);
	hashmap_insert(ob.symbol_map, "irq_install_handler", (uintptr_t)&irq_install_handler);
	hashmap_insert(ob.symbol_map, "printf", (uintptr_t)&printf);

	load_elf("kbrd.drv", &ob, false);

	uint32_t func_address;
	if(hashmap_lookup(ob.symbol_map, "AT_keyboard_init", &func_address))
	{
		((void (*)())func_address)();
	}
	else
	{
		printf("cannot find function\n");
	}
}

void kernel_main()
{
	initialize_video(80, 25);

	for(size_t i = 0; i < _multiboot->m_modsCount; i++)
	{
		uintptr_t rd_begin = ((multiboot_modules*)_multiboot->m_modsAddr)[i].begin;
		uintptr_t rd_end = ((multiboot_modules*)_multiboot->m_modsAddr)[i].end;

		printf("Found Module %X - %X\n", rd_begin, rd_end);
	}

	idt_init();
	isrs_init();
	irqs_init();

	memmanager_init();

	sysclock_init();

	ramdisk_init();

	filesystem_setup_drives();

	setup_syscalls();

	setup_first_task(); //we are now running as a kernel level task

	keyboard_init();

	load_floppy_driver();

	filesystem_set_default_drive(1);

	clear_screen();

	size_t free_mem = memmanager_num_bytes_free();
	size_t total_mem = memmanager_mem_size();

	printf("%u KB free / %u KB Memory\n\n", free_mem / 1024, total_mem / 1024);

	int drive_index = 0;

	directory_handle* current_directory = filesystem_get_root_directory(drive_index);

	if(current_directory == NULL)
	{
		printf("Could not mount root directory for drive %d\n", drive_index);
	}

	load_kbrd_driver();

	spawn_process("shell.elf", WAIT_FOR_PROCESS);
	
	for(;;);
}
