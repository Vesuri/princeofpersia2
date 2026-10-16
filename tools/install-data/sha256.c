/* SHA-256, FIPS 180-4. The installer limits inputs to less than 2 GiB. */
#include "sha256.h"
static const uint32_t k[64]={
0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
static uint32_t rr(uint32_t x,unsigned n) { return (x>>n)|(x<<(32-n)); }
static void transform(SHA256 *s) {
    uint32_t w[16],a=s->h[0],b=s->h[1],c=s->h[2],d=s->h[3],
        e=s->h[4],f=s->h[5],g=s->h[6],h=s->h[7],x,y,t,u;
    unsigned i,j;
    for(i=0;i<16;i++) { const unsigned char *p=s->block+4*i;
        w[i]=((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3]; }
    for(i=0;i<64;i++) {
        j=i&15;
        if(i>=16) { x=w[(i+1)&15]; y=w[(i+14)&15];
            w[j]+=(rr(x,7)^rr(x,18)^(x>>3))+w[(i+9)&15]+(rr(y,17)^rr(y,19)^(y>>10)); }
        t=h+(rr(e,6)^rr(e,11)^rr(e,25))+((e&f)^(~e&g))+k[i]+w[j];
        u=(rr(a,2)^rr(a,13)^rr(a,22))+((a&b)^(a&c)^(b&c));
        h=g; g=f; f=e; e=d+t; d=c; c=b; b=a; a=t+u;
    }
    s->h[0]+=a; s->h[1]+=b; s->h[2]+=c; s->h[3]+=d;
    s->h[4]+=e; s->h[5]+=f; s->h[6]+=g; s->h[7]+=h;
}
void sha_init(SHA256 *s) {
    static const uint32_t initial[8]={0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,
        0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
    unsigned i; for(i=0;i<8;i++) s->h[i]=initial[i]; s->bytes=s->used=0;
}
void sha_update(SHA256 *s,const unsigned char *p,uint32_t n) {
    s->bytes+=n;
    while(n--) { s->block[s->used++]=*p++; if(s->used==64) { transform(s); s->used=0; } }
}
void sha_final(SHA256 *s,unsigned char out[32]) {
    uint32_t lo=s->bytes<<3,hi=s->bytes>>29;
    unsigned i; s->block[s->used++]=128;
    if(s->used>56) { while(s->used<64) s->block[s->used++]=0; transform(s); s->used=0; }
    while(s->used<56) s->block[s->used++]=0;
    for(i=0;i<4;i++) { s->block[56+i]=(unsigned char)(hi>>(24-8*i)); s->block[60+i]=(unsigned char)(lo>>(24-8*i)); }
    transform(s);
    for(i=0;i<32;i++) out[i]=(unsigned char)(s->h[i>>2]>>(24-8*(i&3)));
}
