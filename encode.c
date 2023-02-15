#include "encode.h"

void bin_to_hex(char *dest, unsigned char const *src, size_t src_size)
{
  char const *table = "0123456789abcdef";

  while (src_size--) {
    *dest++ = table[*src >> 4];
    *dest++ = table[*src & 0xf];
    ++src;
  }

  *dest = '\0';
}

void bin_to_base64(char *dest, unsigned char const *src, size_t src_size)
{
  char const *alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                         "abcdefghijklmnopqrstuvwxyz"
                         "0123456789"
                         "+/";
  size_t threes = src_size / 3;
  size_t leftover = src_size % 3;

  while (threes--) {
    *dest++ = alphabet[src[0] >> 2];
    *dest++ = alphabet[((src[0] << 4) | (src[1] >> 4)) & 0x3f];
    *dest++ = alphabet[((src[1] << 2) | (src[2] >> 6)) & 0x3f];
    *dest++ = alphabet[src[2] & 0x3f];
    src += 3;
  }

  switch (leftover) {
  case 2:
    *dest++ = alphabet[src[0] >> 2];
    *dest++ = alphabet[((src[0] << 4) | src[1] >> 4) & 0x3f];
    *dest++ = alphabet[(src[1] << 2) & 0x3f];
    *dest++ = '=';
    break;

  case 1:
    *dest++ = alphabet[src[0] >> 2];
    *dest++ = alphabet[(src[0] << 4) & 0x3f];
    *dest++ = '=';
    *dest++ = '=';
  }

  *dest = '\0';
}
