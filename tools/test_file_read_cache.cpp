#include <cassert>
#include <cstdio>
#include <cstring>
#include "../src/mac/FileReadCache.h"
static uint32_t calls=0,limit=200003,lastBytes=0;
static int32_t failure=0;
static uint8_t value(uint32_t i) { return (i*37+(i>>8))&255; }
static int32_t readAt(void* context,uint32_t offset,uint8_t* out,uint32_t bytes,uint32_t& actual) {
    ++calls;lastBytes=bytes;actual=offset>=limit ? 0 : limit-offset;
    if(actual>bytes)actual=bytes;
    for(uint32_t i=0;i<actual;++i)out[i]=value(offset+i)+(context ? *(uint8_t*)context : 0);
    return failure;
}
static void check(const uint8_t* out,uint32_t offset,uint32_t bytes) {
    for(uint32_t i=0;i<bytes;++i)assert(out[i]==value(offset+i));
}
int main() {
    uint8_t cache[65536],out[131089];uint32_t actual=99;
    FileReadCache c;c.bind(cache,200003,readAt,0);
    assert(c.read(0,out,16,actual)==0 && actual==16 && calls==1 && lastBytes==65536);check(out,0,16);
    assert(c.read(16,out,16,actual)==0 && calls==1);check(out,16,16);
    assert(c.read(65530,out,20,actual)==0 && calls==2);check(out,65530,20);
    assert(c.read(1000,out,sizeof(out),actual)==0 && calls==3 && lastBytes==sizeof(out));check(out,1000,actual);
    assert(c.read(199996,out,20,actual)==-39 && actual==7 && calls==4);check(out,199996,7);
    assert(c.read(200003,out,1,actual)==-39 && actual==0 && calls==4);
    assert(c.read(200003,0,0,actual)==0 && actual==0);
    assert(c.read(0,0,1,actual)==-50 && actual==0);
    assert(c.read(0,out,0x80000000,actual)==-50 && actual==0);
    // A failed fill cannot turn into a later cache hit; partial bytes are accounted.
    c.bind(cache,200003,readAt,0);limit=7;failure=-36;
    assert(c.read(0,out,20,actual)==-36 && actual==7);check(out,0,7);
    auto before=calls;assert(c.read(0,out,1,actual)==-36 && calls==before+1);
    failure=0;c.bind(cache,200003,readAt,0);
    assert(c.read(0,out,20,actual)==-39 && actual==7);
    // Separate forks never share cached data or bounds.
    uint8_t second[65536];FileReadCache d;d.bind(second,0,readAt,0);
    before=calls;assert(d.read(0,out,1,actual)==-39 && calls==before);
    limit=200003;c.bind(cache,limit,readAt,0);
    assert(c.read(0,out,16,actual)==0);check(out,0,16);
    uint8_t seed=73;d.bind(second,limit,readAt,&seed);
    assert(d.read(0,out,16,actual)==0);
    for(uint32_t i=0;i<16;++i)assert(out[i]==(uint8_t)(value(i)+seed));
    before=calls;assert(c.read(0,out,16,actual)==0 && calls==before);check(out,0,16);
    puts("PASS file-read-cache: hits, crossing, direct large read, EOF, short/error fills, isolated forks");
}
