#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>

#include <kernel/filesystem.h>
#include <kernel/interrupt.h>
#include <kernel/memorymanager.h>
#include <kernel/multiboot.h>
#include <kernel/task.h>
#include <kernel/elf.h>
#include <drivers/video.h>
#include <drivers/sysclock.h>
#include <drivers/kbrd.h>
#include <drivers/rs232.h>
#include <drivers/floppy.h>
#include <drivers/ramdisk.h>
#include <drivers/isa_dma.h>

extern multiboot_info* _multiboot;

extern void _IMAGE_END_;
extern void _BSS_END_;
extern void _DATA_END_;

typedef struct {
	const char* name;
	void* address;
} func_info;

void load_driver(const char* filename, const char* func_name, 
				 const func_info* functions, size_t num_functions)
{
	dynamic_object ob;
	ob.lib_set = hashmap_create(16);
	ob.symbol_map = hashmap_create(16);
	ob.glob_data_symbol_map = hashmap_create(16);

	for(size_t i = 0; i < num_functions; i++)
	{
		//printf("key = %s, value = %X\n", functions[i].name, (uint32_t)functions[i].address);
		hashmap_insert(ob.symbol_map, 
					   functions[i].name, (uint32_t)functions[i].address);
	}

	//hashmap_print(ob.symbol_map);

	load_elf(filename, &ob, false);

	uint32_t func_address;
	if(hashmap_lookup(ob.symbol_map, func_name, &func_address))
	{
		((void (*)())func_address)();
	}
	else
	{
		printf("cannot find function\n");
	}
}

void load_floppy_driver()
{
	func_info list[] = {
		{"irq_install_handler", &irq_install_handler},
		{"printf", &printf},
		{"filesystem_add_drive", &filesystem_add_drive},
		{"sysclock_sleep", &sysclock_sleep},
		{"isa_dma_begin_transfer", &isa_dma_begin_transfer}
	};

	load_driver("floppy.drv", "floppy_init", list, sizeof(list) / sizeof(func_info));
}

void load_kbrd_driver()
{
	func_info list[] = {
		{"irq_install_handler", &irq_install_handler},
		{"printf", &printf},
		{"handle_keyevent", &handle_keyevent}
	};

	load_driver("kbrd.drv", "AT_keyboard_init", list, sizeof(list) / sizeof(func_info));
}

void kernel_main()
{
	initialize_video(80, 25);

	clear_screen();

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
