//https://forums.freertos.org/t/thread-safe-memory-allocation/8003/2

#include <cassert>
#include "freertos/FreeRTOS.h"

void* operator new( size_t size )
{
    auto ptr = pvPortMalloc(size);
    assert(ptr);
    return ptr;
}

void* operator new[]( size_t size )
{
    auto ptr = pvPortMalloc(size);
    assert(ptr);
    return ptr;
}

void operator delete( void * ptr )
{
    vPortFree(ptr);
}

void operator delete[]( void * ptr )
{
    vPortFree(ptr);
}


