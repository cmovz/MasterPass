#include "cleanse.h"

void cleanse(volatile void* mem, size_t size)
{
  volatile unsigned char* ptr = mem;

  while(size--)
    *ptr++ = 0x00;
}
