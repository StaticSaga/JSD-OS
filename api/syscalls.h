#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <stdint.h>
#include <stddef.h>

#define AS_TYPE(t, a) (*((t*)&a))
#define AS_UINT32(a) (*((uint32_t*)&a))

enum syscall_indices
{
	SYSCALL_PRINT = 0,
	SYSCALL_OPEN = 1,
	SYSCALL_CLOSE = 2,
	SYSCALL_READ = 3,
	SYSCALL_EXIT = 4
};

struct file_stream;
typedef struct file_stream file_stream;

static inline uint32_t do_syscall_3(size_t   syscall_index, uint32_t arg0, uint32_t arg1, uint32_t arg2)
{
	uint32_t retval;
	__asm__ volatile(	"int $0x80"
						:"=a"(retval), "+c"(arg1), "+d"(arg2)
						:"b"(syscall_index), "a"(arg0)
						:"memory");
	return retval;
}

static inline uint32_t do_syscall_2(size_t syscall_index, uint32_t arg0, uint32_t arg1)
{
	uint32_t retval;
	__asm__ volatile(	"int $0x80"
						:"=a"(retval), "+c"(arg1)
						:"b"(syscall_index), "a"(arg0)
						:"%edx", "memory");
	return retval;
}

//"int 0x80" parm[ebx] [eax] value [eax] modifies [ecx edx];
static inline uint32_t do_syscall_1(size_t syscall_index, const uint32_t arg)
{
	uint32_t retval;
	__asm__ volatile(	"int $0x80"
						:"=a"(retval)
						:"b"(syscall_index), "a"(arg)
						:"%ecx", "%edx", "memory");
	return retval;
}

static inline void exit(int a)
{
	do_syscall_1(SYSCALL_EXIT, (uint32_t)a);
}

static inline void print_string(const char *a)
{
	do_syscall_1(SYSCALL_PRINT, (uint32_t)a);
}

static inline file_stream* open(const char *path, int flags)
{
	return (file_stream*)do_syscall_2(SYSCALL_OPEN, (uint32_t)path, (uint32_t)flags);
}

static inline int close(file_stream* file)
{
	return (int)do_syscall_1(SYSCALL_CLOSE, (uint32_t)file);
}

static inline int read(void* dst, size_t len, file_stream* file)
{
	return (int)do_syscall_3(SYSCALL_READ, (uint32_t)dst, (uint32_t)len, (uint32_t)file);
}


#endif