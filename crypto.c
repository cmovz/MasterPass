#include "crypto.h"
#include "hash.h"
#include "encode.h"
#include "cleanse.h"
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

/* this determines how hard it will be to brute-force the password, ideally the
  user will have a strong password since it's the only one they have to remember
  but it's still a good idea to make it harder for an attacker to try and crack
  weak passwords */
#define KEY_DERIVATION_ITERATIONS (1024*1024)

/* sha256("THIS IS THE PUBLIC TYPE, THESE HASHES MIGHT BECOME PUBLIC") */
static unsigned char const public_type[32] = {
  0x4c,0x67,0x71,0x17,0xde,0xc2,0xb9,0xab,0x76,0xd3,0xcb,0xd6,0x8f,0x7b,0x6b,
  0x37,0x2b,0xc4,0x55,0x66,0x93,0x96,0xc0,0x4a,0xe5,0xc4,0x68,0xc0,0x36,0xc7,
  0x2f,0x17,
};

/* sha256("THIS IS THE PRIVATE TYPE, THESE HASHES WILL BECOME PASSWORDS") */
static unsigned char const private_type[32] = {
  0x07,0xee,0x61,0x3c,0x9c,0x14,0x70,0xad,0x62,0x40,0xe8,0x56,0x04,0x3e,0x54,
  0xe0,0xe7,0xf7,0x15,0x0b,0x79,0x0d,0x52,0xb2,0x04,0xb5,0xd3,0xac,0x8a,0xf1,
  0xaa,0xf2,
};

/* transition state */
static void random_oracle_f(RandomOracle* ro)
{
  hash(ro->state, (void const*)ro, sizeof *ro);
}

/* Public */
void crypto_init(Crypto* crypto)
{
  crypto->has_key = 0;
}

/* clean up */
void crypto_destroy(Crypto* crypto)
{
  /*crypto->has_key = 0;*/
  cleanse(crypto, sizeof *crypto);
}

void crypto_set_password(Crypto* crypto, unsigned char const* password,
                         size_t size)
{
  int i;

  memset(crypto->oracle.state, 0x00, CRYPTO_KEY_SIZE);
  hash(crypto->oracle.key, password, size);

  for(i = 0; i < KEY_DERIVATION_ITERATIONS; ++i)
    random_oracle_f(&crypto->oracle);

  memcpy(crypto->generator.key, crypto->oracle.state, CRYPTO_KEY_SIZE);
  crypto->has_key = 1;
}

void crypto_reset_password(Crypto* crypto)
{
  /*crypto->hash_key = 0;*/
  cleanse(crypto, sizeof *crypto);
}

void crypto_generate_public_hash(Crypto* crypto,
                                 unsigned char* dest_32_bytes,
                                 unsigned char const* place, size_t place_size,
                                 unsigned char const* login, size_t login_size)
{
  if(!crypto->has_key)
    abort();

  hash(crypto->generator.place, place, place_size);
  hash(crypto->generator.login, login, login_size);

  /* version is meaningless for public hashes */
  memset(crypto->generator.version, 0x00, PASSWORD_VERSION_SIZE);

  memcpy(crypto->generator.type, public_type, TYPE_SIZE);

  hash(dest_32_bytes, (void const*)&crypto->generator,
       sizeof crypto->generator);
}

void crypto_generate_password(Crypto* crypto,
                              char* base64_dest,
                              char* hex_dest,
                              unsigned char const* place, size_t place_size,
                              unsigned char const* login, size_t login_size,
                              unsigned char const* version)
{
  if(!crypto->has_key)
    abort();

  hash(crypto->generator.place, place, place_size);
  hash(crypto->generator.login, login, login_size);
  memcpy(crypto->generator.version, version, PASSWORD_VERSION_SIZE);
  memcpy(crypto->generator.type, private_type, TYPE_SIZE);

  memset(crypto->oracle.state, 0x00, CRYPTO_KEY_SIZE);
  hash(crypto->oracle.key, (void const*)&crypto->generator,
       sizeof crypto->generator);

  random_oracle_f(&crypto->oracle);

  bin_to_base64(base64_dest, crypto->oracle.state, CRYPTO_KEY_SIZE);
  bin_to_hex(hex_dest, crypto->oracle.state, CRYPTO_KEY_SIZE);
}
