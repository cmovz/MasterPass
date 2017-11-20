#ifndef RANDOM_ORACLE_H
#define RANDOM_ORACLE_H

#include "hash.h"

#define RANDOM_ORACLE_KEY_SIZE HASH_MD_SIZE

typedef struct RandomOracle {
  unsigned char state[RANDOM_ORACLE_KEY_SIZE];
  unsigned char key[RANDOM_ORACLE_KEY_SIZE];
  int pos;
} RandomOracle;

void random_oracle_init(RandomOracle* ro, unsigned char const* key);
void random_oracle_destroy(RandomOracle* ro);

void random_oracle_get_bytes(RandomOracle* ro, void* dest, size_t n);

/* get 1 random byte [0x00;0xff]*/
int random_oracle_pop(RandomOracle* ro);

#endif
