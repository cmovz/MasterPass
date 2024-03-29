#ifndef CRYPTO_H
#define CRYPTO_H

#define CRYPTO_KEY_SIZE 32
#define PASSWORD_VERSION_SIZE 16
#define TYPE_SIZE 16

#include "random_oracle.h"
#include <stddef.h>
#include <stdint.h>

typedef struct KeyGenerator {
  unsigned char key[CRYPTO_KEY_SIZE]; /* deterministic password-derived key */
  unsigned char place[CRYPTO_KEY_SIZE]; /* hash of site's or whatever's name */
  unsigned char login[CRYPTO_KEY_SIZE]; /* hash of username */
  unsigned char version[PASSWORD_VERSION_SIZE]; /* little-endian counter */

  /* this separates passwords from unprotected data such as keys and values that
    hold the current password version since password key will come from:
      sha256(key, place=site, login=username, version=n, type=private);
    and storage key from:
      sha256(key, place=site, login=username, version=n, type=public)*/
  unsigned char type[TYPE_SIZE];
} KeyGenerator;

typedef struct Crypto {
  KeyGenerator generator;
  RandomOracle oracle;
  int has_key;
} Crypto;

void crypto_init(Crypto *crypto);

/* clean up */
void crypto_destroy(Crypto *crypto);

void crypto_set_password(Crypto *crypto, unsigned char const *password,
                         size_t size);

void crypto_reset_password(Crypto *crypto);

/* maybe refactor below to use WorkingInput,
  TODO: rename */
void crypto_generate_public_hash(Crypto *crypto,
                                 unsigned char *dest_32_bytes,
                                 unsigned char const *place, size_t place_size,
                                 unsigned char const *login, size_t login_size);

void crypto_generate_password(Crypto *crypto,
                              char *base64_dest_45_bytes,
                              char *hex_dest_65_bytes,
                              unsigned char const *place, size_t place_size,
                              unsigned char const *login, size_t login_size,
                              unsigned char const *version_16bytes);

#endif
