#pragma once
#ifndef __mc68000__
#include <stdint.h>
#endif

// Shared host/native fixture. Buffers are static: never consume the trap stack.
// Compare all bytes with an independent byte-wise memmove, including canaries.
namespace BlockMoveChecks {
static uint8_t actual[307328],expected[307328];
template<class Move> uint32_t run(Move move) {
    uint32_t checks=0;
    auto check=[&](uint32_t src,uint32_t dst,uint32_t n,uint32_t size) {
        for(uint32_t i=0;i<size;++i) actual[i]=expected[i]=uint8_t(i*73+(i>>8)*37);
        if(dst>src) for(uint32_t i=n;i;--i) expected[dst+i-1]=expected[src+i-1];
        else for(uint32_t i=0;i<n;++i) expected[dst+i]=expected[src+i];
        move(actual+src,actual+dst,n);
        for(uint32_t i=0;i<size;++i) if(actual[i]!=expected[i]) return false;
        ++checks;return true;
    };
    // Every alignment, overlap distance and tail around 4/32/64/96 bytes; larger boundaries below.
    for(uint32_t n=0;n<=100;++n)
        for(uint32_t src=40;src<44;++src)
            for(uint32_t dst=8;dst<=76;++dst)
                if(!check(src,dst,n,184)) return 0;
    // Large loop boundaries, disjoint ranges, both overlapping directions.
    const uint32_t lengths[]={127,128,129,255,256,257,65535,65536,65537,262143,262144,307200};
    for(uint32_t n:lengths) for(uint32_t align=0;align<4;++align)
        for(uint32_t gap=1;gap<=33;gap+=8) {
            if(!check(8+align,8+align+gap,n,n+96))return 0;
            if(!check(8+align+gap,8+align,n,n+96))return 0;
        }
    for(uint32_t a=0;a<4;++a)for(uint32_t b=0;b<4;++b) {
        if(!check(8+a,2048+b,1024,4096))return 0;
        if(!check(2048+a,8+b,1024,4096))return 0;
    }
    return checks;
}
}
