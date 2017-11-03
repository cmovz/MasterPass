#include "hash.h"
#include "sha256.h"

void hash(void* md, void const* m, size_t size)
{
  sha256(m, size, md);
}
