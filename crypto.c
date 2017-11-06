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

/* sha256("public") */
static unsigned char const public_type[32] = {
  0xef,0xa1,0xf3,0x75,0xd7,0x61,0x94,0xfa,0x51,0xa3,0x55,0x6a,0x97,0xe6,0x41,
  0xe6,0x16,0x85,0xf9,0x14,0xd4,0x46,0x97,0x9d,0xa5,0x0a,0x55,0x1a,0x43,0x33,
  0xff,0xd7,
};

/* sha256("private") */
static unsigned char const private_type[32] = {
  0x71,0x5d,0xc8,0x49,0x3c,0x36,0x57,0x9a,0x5b,0x11,0x69,0x95,0x10,0x0f,0x63,
  0x5e,0x35,0x72,0xfd,0xf8,0x70,0x3e,0x70,0x8e,0xf1,0xa0,0x8d,0x94,0x3b,0x36,
  0x77,0x4e,
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
