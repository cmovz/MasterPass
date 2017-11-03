#include "sha256.h"
#include "cleanse.h"

#define ROR(x,n) ((x >> n)|(x << (32-n)))
#define S1(e) (ROR(e,6) ^ ROR(e,11) ^ ROR(e,25))
#define CH(e,f,g) ((e & f) ^ ((~e) & g))
#define S0(a) (ROR(a,2) ^ ROR(a,13) ^ ROR(a,22))
#define MAJ(a,b,c) ((a & b) ^ (a & c) ^ (b & c))
#define TEMP2(a,b,c) (S0(a) + MAJ(a,b,c))
#define s0(c,i) (ROR(c->w[i-15],7) ^ ROR(c->w[i-15],18) ^ (c->w[i-15] >> 3))
#define s1(c,i) (ROR(c->w[i-2],17) ^ ROR(c->w[i-2],19) ^ (c->w[i-2] >> 10))

typedef uint64_t u64;
typedef uint32_t u32;

/* know exactly where to cleanse later */
typedef struct SHA256_CTX {
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
} SHA256_CTX;

static u32 const k[64] = {
  0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,
  0xab1c5ed5,0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,
  0x9bdc06a7,0xc19bf174,0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,
  0x4a7484aa,0x5cb0a9dc,0x76f988da,0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,
  0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,0x27b70a85,0x2e1b2138,0x4d2c6dfc,
  0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,0xa2bfe8a1,0xa81a664b,
  0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,0x19a4c116,
  0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
  0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,
  0xc67178f2
};

static void sha256_compress(SHA256_CTX* c, void const* block)
{
  /* non-sensitive data doesn't have to be cleansed */
  unsigned char const* src = block;
  int i;

  for(i = 0; i < 64; ++i){
    c->w[i] = (src[0] << 24) | (src[1] << 16) | (src[2] << 8) | src[3];
    src += 4;
  }

  for(i = 16; i < 64; ++i)
    c->w[i] = c->w[i-16] + s0(c,i) + c->w[i-7] + s1(c,i);

  c->a = c->hs[0];
  c->b = c->hs[1];
  c->c = c->hs[2];
  c->d = c->hs[3];
  c->e = c->hs[4];
  c->f = c->hs[5];
  c->g = c->hs[6];
  c->h = c->hs[7];

  for(i = 0; i < 64; ++i){
    c->temp1 = c->h + S1(c->e) + CH(c->e,c->f,c->g) + k[i] + c->w[i];
    c->temp2 = TEMP2(c->a,c->b,c->c); /*FIXME either TEMP1() or do comp here */

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

void sha256(unsigned char const* m, u64 m_size, unsigned char* md)
{
  /* sensitive */
  unsigned char last_chunk[128];
  SHA256_CTX ctx;

  /* non-sensitive */
  u64 chunks = m_size >> 6;
  u64 L = m_size << 3;
  int rem = m_size & 63;
  int total_size = (rem + 1 + 8 + 63) & ~63;
  int i;

  ctx.hs[0] = 0x6a09e667;
  ctx.hs[1] = 0xbb67ae85;
  ctx.hs[2] = 0x3c6ef372;
  ctx.hs[3] = 0xa54ff53a;
  ctx.hs[4] = 0x510e527f;
  ctx.hs[5] = 0x9b05688c;
  ctx.hs[6] = 0x1f83d9ab;
  ctx.hs[7] = 0x5be0cd19;

  while(chunks){
    sha256_compress(&ctx, m);
    m += 64;
    --chunks;
  }

  for(i = 0; i < rem; ++i)
    last_chunk[i] = m[i];

  last_chunk[rem] = 0x80;

  for(i = rem + 1; i < total_size - 8; ++i)
    last_chunk[i] = 0x00;

  for(i = 8; i > 0; --i)
    last_chunk[total_size - i] = L >> (i * 8 - 8);

  sha256_compress(&ctx, last_chunk);

  if(total_size == 128)
    sha256_compress(&ctx, last_chunk + 64);

  for(i = 0; i < 8; ++i){
    md[i * 4    ] = ctx.hs[i] >> 24;
    md[i * 4 + 1] = ctx.hs[i] >> 16;
    md[i * 4 + 2] = ctx.hs[i] >>  8;
    md[i * 4 + 3] = ctx.hs[i]      ;
  }

  /* clean up the stack later for noticeably better performance
  cleanse(&ctx, sizeof ctx);
  cleanse(last_chunk, sizeof last_chunk);
  */
}
