/* HMAC using SHA256 as described on:
    http://cseweb.ucsd.edu/~mihir/papers/hmac-cb.pdf
*/

#ifndef HMAC_H
#define HMAC_H

#include "sha256.h"
#include <stddef.h>

#define HMAC_HASH_BLOCK_SIZE SHA256_BLOCK_SIZE
#define HMAC_KEY_SIZE 32
#define HMAC_HASH_SIZE SHA256_MD_SIZE
#define HMAC_SIZE HMAC_HASH_SIZE

typedef struct {
  unsigned char opad[HMAC_HASH_BLOCK_SIZE];
  SHA256_CTX ctx;
} HMAC;

void hmac_init(HMAC *hmac, unsigned char const *key);

void hmac_update(HMAC *hmac, void const *data, size_t data_size);

/* return HMAC and clean up the structure */
void hmac_final(HMAC *hmac, unsigned char *out);

#endif
