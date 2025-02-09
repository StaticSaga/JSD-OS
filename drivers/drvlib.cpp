extern "C" {
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
}

void* operator new (size_t size)
{
	return malloc(size);
}

void* operator new[](size_t size)
{
	return malloc(size);
}

void operator delete(void* p)
{
	free(p);
}

void operator delete[](void* p)
{
	free(p);
}