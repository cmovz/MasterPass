#ifndef SHA256_H
#define SHA256_H

#include <stdint.h>

void sha256(unsigned char const* m, uint64_t m_size, unsigned char* md);

#endif
