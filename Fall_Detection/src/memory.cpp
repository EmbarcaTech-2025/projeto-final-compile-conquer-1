#include "FreeRTOS.h"
#include "memory.h"
#include <cstring>

// override Edge Impulse memory allocation to use FreeRTOS heap
void *ei_malloc(size_t size)
{
    return pvPortMalloc(size);
}

void *ei_calloc(size_t nitems, size_t size)
{
    void *ptr = pvPortMalloc(nitems * size);
    if (ptr)
    {
        memset(ptr, 0, nitems * size);
    }
    return ptr;
}

void ei_free(void *ptr)
{
    if (ptr)
    {
        vPortFree(ptr);
    }
}

// override standard allocators for Mongoose

void *malloc(size_t size)
{
    return pvPortMalloc(size);
}

void *calloc(size_t nitems, size_t size)
{
    void *ptr = pvPortMalloc(nitems * size);
    if (ptr)
    {
        memset(ptr, 0, nitems * size);
    }
    return ptr;
}

void free(void *ptr)
{
    if (ptr)
    {
        vPortFree(ptr);
    }
}
