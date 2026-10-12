#ifndef POP2_SHA256_H
#define POP2_SHA256_H
#ifndef POP2_PLATFORM_AMIGA
#include <cstdint>
#endif
typedef struct { uint32_t h[8],bytes,used; unsigned char block[64]; } SHA256;
void sha_init(SHA256 *s);
void sha_update(SHA256 *s,const unsigned char *p,uint32_t n);
void sha_final(SHA256 *s,unsigned char out[32]);
#endif
