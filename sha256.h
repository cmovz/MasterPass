#ifndef SHA256_H
#define SHA256_H

#include <stdint.h>

#define SHA256_BLOCK_SIZE 64
#define SHA256_MD_SIZE 32

/* this is required by the HMAC code to avoid buffering too much */
typedef struct {
  uint32_t hs[8];
  unsigned char buffer[SHA256_BLOCK_SIZE];
  uint64_t size;
  uint32_t pos;
} SHA256_CTX;

void sha256(void const* m, uint64_t m_size, void* md);

void sha256_init(SHA256_CTX* ctx);
void sha256_update(SHA256_CTX* ctx, void const* data, uint64_t size);
void sha256_final(SHA256_CTX* ctx, void* md);

#endif
