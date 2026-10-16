#ifndef VETTE_INSTALL_SHA256_H
#define VETTE_INSTALL_SHA256_H
#include "io.h"
typedef struct { uint32_t h[8],bytes,used; unsigned char block[64]; } SHA256;
void sha_init(SHA256 *s);
void sha_update(SHA256 *s,const unsigned char *p,uint32_t n);
void sha_final(SHA256 *s,unsigned char out[32]);
#endif
