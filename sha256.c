#include "sha256.h"
#include "cleanse.h"
#include <string.h>

#define ROR(x, n) ((x >> n)|(x << (32-n)))
#define S1(e) (ROR(e,6) ^ ROR(e,11) ^ ROR(e,25))
#define CH(e, f, g) ((e & f) ^ ((~e) & g))
#define S0(a) (ROR(a,2) ^ ROR(a,13) ^ ROR(a,22))
#define MAJ(a, b, c) ((a & b) ^ (a & c) ^ (b & c))
#define s0(c, i) (ROR(c->w[i-15],7) ^ ROR(c->w[i-15],18) ^ (c->w[i-15] >> 3))
#define s1(c, i) (ROR(c->w[i-2],17) ^ ROR(c->w[i-2],19) ^ (c->w[i-2] >> 10))

typedef uint64_t u64;
typedef uint32_t u32;

/* know exactly where to cleanse later */
typedef struct SHA256_INTERNAL_CTX {
  u32 hs[8];
  u32 w[64];
  u32 a;
  u32 b;
  u32 c;
  u32 d;
  u32 e;
  u32 f;
  u32 g;
  u32 h;
  u32 temp1;
  u32 temp2;
} SHA256_INTERNAL_CTX;

static u32 const k[64] = {
  0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1,
  0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
  0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786,
  0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
  0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
  0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
  0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
  0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
  0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a,
  0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
  0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

static void le_to_be(void *dest_ptr, void const *be, int size)
{
  unsigned char const *src = be;
  unsigned char *dest = dest_ptr;
  int i;

  for (i = size; i > 0; --i)
    dest[size - i] = src[i - 1];
}

static void sha256_compress(SHA256_INTERNAL_CTX *c, void const *block)
{
  /* non-sensitive data doesn't have to be cleansed */
  unsigned char const *src = block;
  int i;

  for (i = 0; i < 16; ++i) {
    le_to_be(c->w + i, src, sizeof c->w[0]);
    src += sizeof c->w[0];
  }

  for (i = 16; i < 64; ++i)
    c->w[i] = c->w[i - 16] + s0(c, i) + c->w[i - 7] + s1(c, i);

  c->a = c->hs[0];
  c->b = c->hs[1];
  c->c = c->hs[2];
  c->d = c->hs[3];
  c->e = c->hs[4];
  c->f = c->hs[5];
  c->g = c->hs[6];
  c->h = c->hs[7];

  for (i = 0; i < 64; ++i) {
    c->temp1 = c->h + S1(c->e) + CH(c->e, c->f, c->g) + k[i] + c->w[i];
    c->temp2 = S0(c->a) + MAJ(c->a, c->b, c->c);

    c->h = c->g;
    c->g = c->f;
    c->f = c->e;
    c->e = c->d + c->temp1;
    c->d = c->c;
    c->c = c->b;
    c->b = c->a;
    c->a = c->temp1 + c->temp2;
  }

  c->hs[0] += c->a;
  c->hs[1] += c->b;
  c->hs[2] += c->c;
  c->hs[3] += c->d;
  c->hs[4] += c->e;
  c->hs[5] += c->f;
  c->hs[6] += c->g;
  c->hs[7] += c->h;
}

/* public */

void sha256_init(SHA256_CTX *ctx)
{
  ctx->hs[0] = 0x6a09e667;
  ctx->hs[1] = 0xbb67ae85;
  ctx->hs[2] = 0x3c6ef372;
  ctx->hs[3] = 0xa54ff53a;
  ctx->hs[4] = 0x510e527f;
  ctx->hs[5] = 0x9b05688c;
  ctx->hs[6] = 0x1f83d9ab;
  ctx->hs[7] = 0x5be0cd19;

  ctx->pos = 0;
  ctx->size = 0;
}

void sha256_update(SHA256_CTX *ctx, void const *data, uint64_t size)
{
  SHA256_INTERNAL_CTX ictx;
  size_t rem = sizeof ctx->buffer - ctx->pos;
  size_t n = (size < rem) ? size : rem;
  size_t source_blocks;

  ctx->size += size;

  /* I'll benchmark this later to decide if it's a good idea when size == 64 in
  a lot of consecutive computations
  if((0 == ctx->pos) && (SHA256_BLOCK_SIZE == size)){
    memcpy(ictx.hs, ctx->hs, sizeof ictx.hs);
    sha256_compress(&ictx, data);
    memcpy(ctx->hs, ictx.hs, sizeof ctx->hs);
    return;
  }*/

  memcpy(ctx->buffer + ctx->pos, data, n);
  ctx->pos += n;

  /* nothing else to do here */
  if (SHA256_BLOCK_SIZE != ctx->pos)
    return;

  /* consume buffer */
  memcpy(ictx.hs, ctx->hs, sizeof ictx.hs);
  sha256_compress(&ictx, ctx->buffer);

  /* consume any other data directly from source */
  size -= n;
  data = (unsigned char const *) data + n;
  source_blocks = size / SHA256_BLOCK_SIZE;
  ctx->pos = size % SHA256_BLOCK_SIZE;
  while (source_blocks--) {
    sha256_compress(&ictx, data);
    data = (unsigned char const *) data + SHA256_BLOCK_SIZE;
  }

  /* keep any data that doesn't fill a block */
  memcpy(ctx->buffer, data, ctx->pos);

  /* bring state back to public ctx */
  memcpy(ctx->hs, ictx.hs, sizeof ctx->hs);
}

void sha256_final(SHA256_CTX *ctx, void *md)
{
  SHA256_INTERNAL_CTX ictx;
  unsigned char last_block[SHA256_BLOCK_SIZE];
  uint64_t L = ctx->size * 8;
  int i;

  memcpy(ictx.hs, ctx->hs, sizeof ictx.hs);
  ctx->buffer[ctx->pos++] = 0x80;
  memset(ctx->buffer + ctx->pos, 0x00, SHA256_BLOCK_SIZE - ctx->pos);

  /* fits? */
  if (ctx->pos + sizeof L <= SHA256_BLOCK_SIZE) {
    le_to_be(ctx->buffer + SHA256_BLOCK_SIZE - sizeof L, &L, sizeof L);
    sha256_compress(&ictx, ctx->buffer);
  } else {
    sha256_compress(&ictx, ctx->buffer);
    memset(last_block, 0x00, SHA256_BLOCK_SIZE - sizeof L);
    le_to_be(last_block + SHA256_BLOCK_SIZE - sizeof L, &L, sizeof L);
    sha256_compress(&ictx, last_block);
  }

  for (i = 0; i < sizeof ictx.hs / sizeof ictx.hs[0]; ++i) {
    le_to_be(md, ictx.hs + i, sizeof ictx.hs[0]);
    md = (unsigned char *) md + sizeof ictx.hs[0];
  }
}

void sha256(void const *m_ptr, uint64_t m_size, void *md_ptr)
{
  SHA256_CTX ctx;

  sha256_init(&ctx);
  sha256_update(&ctx, m_ptr, m_size);
  sha256_final(&ctx, md_ptr);
}
