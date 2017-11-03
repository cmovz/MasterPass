#ifndef ENCODE_H
#define ENCODE_H

#include <stddef.h>

void bin_to_hex(char* dest, unsigned char const* src, size_t src_size);
void bin_to_base64(char* dest, unsigned char const* src, size_t src_size);

#endif
