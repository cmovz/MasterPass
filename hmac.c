#include "hmac.h"
#include "cleanse.h"
#include <string.h>

/* private */
static void xor_into(unsigned char* dest, unsigned char const* src,
                     unsigned char x, size_t n)
{
  while(n--)
    *dest++ = *src++ ^ x;
}

/* public */
void HMAC_init(HMAC* hmac, unsigned char const* key)
{
  unsigned char ipad[HMAC_HASH_BLOCK_SIZE];

  /* opad */
  xor_into(hmac->opad, key, 0x5c, HMAC_KEY_SIZE);
  memset(hmac->opad + HMAC_KEY_SIZE, 0x5c, sizeof hmac->opad - HMAC_KEY_SIZE);

  /* ipad */
  xor_into(ipad, key, 0x36, HMAC_KEY_SIZE);
  memset(ipad + HMAC_KEY_SIZE, 0x36, sizeof ipad - HMAC_KEY_SIZE);

  sha256_init(&hmac->ctx);
  sha256_update(&hmac->ctx, ipad, sizeof ipad);
}

void HMAC_update(HMAC* hmac, void const* data, size_t data_size)
{
  sha256_update(&hmac->ctx, data, data_size);
}

/* return HMAC and clean up the structure */
void HMAC_final(HMAC* hmac, unsigned char* out)
{
  unsigned char final_data[sizeof hmac->opad + HMAC_HASH_SIZE];

  memcpy(final_data, hmac->opad, sizeof hmac->opad);
  sha256_final(&hmac->ctx, final_data + sizeof hmac->opad);
  sha256(final_data, sizeof final_data, out);

  /* could be removed for consistency, but would have to be reintroduced if
    cleaning policy changed, no performance difference, so leave it here */
  cleanse(hmac, sizeof *hmac);
}
