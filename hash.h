#ifndef HASH_H
#define HASH_H

#include <stddef.h>

#define HASH_MD_SIZE 32

/* any secure hash function works, m and md must be allowed to overlap */
void hash(void* md, void const* m, size_t size);

#endif
