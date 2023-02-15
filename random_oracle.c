#include "random_oracle.h"
#include "cleanse.h"
#include <string.h>
#include <assert.h>

/* transition state */
static void random_oracle_f(RandomOracle *ro)
{
  hash(ro->state, (void const *) ro, RANDOM_ORACLE_KEY_SIZE * 2);
}

void random_oracle_destroy(RandomOracle *ro)
{
  cleanse(ro, sizeof *ro);
}

/* public */
void random_oracle_init(RandomOracle *ro, unsigned char const *key)
{
  memset(ro->state, 0x00, RANDOM_ORACLE_KEY_SIZE);
  memcpy(ro->key, key, RANDOM_ORACLE_KEY_SIZE);
  ro->pos = RANDOM_ORACLE_KEY_SIZE;
}

void random_oracle_transition_state(RandomOracle *ro)
{
  random_oracle_f(ro);
  ro->pos = 0;
}

void random_oracle_get_bytes(RandomOracle *ro, void *dest, size_t n)
{
  size_t full_cycles;
  size_t take_n;
  size_t left = RANDOM_ORACLE_KEY_SIZE - ro->pos;

  /* use buffer */
  if (ro->pos != RANDOM_ORACLE_KEY_SIZE) {
    /* fill everything from buffer */
    if (n <= left) {
      memcpy(dest, ro->state + ro->pos, n);
      ro->pos += n;
      return;
    }

    /* or buffer will be depleted */
    memcpy(dest, ro->state + ro->pos, left);
    n -= left;
    dest = (char *) dest + left;
  }

  /* buffer is empty */
  full_cycles = n / RANDOM_ORACLE_KEY_SIZE;
  take_n = n % RANDOM_ORACLE_KEY_SIZE;

  /* consume entire buffers */
  while (full_cycles--) {
    random_oracle_f(ro);
    memcpy(dest, ro->state, RANDOM_ORACLE_KEY_SIZE);
    dest = (char *) dest + RANDOM_ORACLE_KEY_SIZE;
  }

  /* buffer is empty */
  random_oracle_f(ro);

  memcpy(dest, ro->state, take_n);
  ro->pos = take_n;
}

int random_oracle_pop(RandomOracle *ro)
{
  if (RANDOM_ORACLE_KEY_SIZE == ro->pos) {
    random_oracle_f(ro);
    ro->pos = 0;
  }

  return ro->state[ro->pos++];
}
