#include <initializer_list>
#include <cstdint>
#include <cassert>
#include <cstdio>
#include <cstring>
#define private public
#include "mac/MacHeap.h"
#undef private
static void compare(MacHeap& h) {
    assert(h.check());
    for(uint32_t size: {0u,1u,8u,31u,256u,1024u,65536u,0xffffffffu}) {
        uint32_t expected=0;
        if(size<0x7fffffe0u) {
            uint32_t needed=(size+31)&~7u;if(needed<32)needed=32;
            for(uint32_t off=64;off<h.end_;off+=h.block(off).span)
                if(h.block(off).kind==0 && h.block(off).span>=needed){expected=off;break;}
        }
        assert(h.findSpace(size)==expected);assert(h.check());
    }
}
int main() {
    alignas(8) static uint8_t arena[65536];MacHeap h;
    // Shrinking before the hint creates a new earlier first-fit hole.
    assert(h.init(arena,sizeof arena));auto a=h.newHandle(1024);auto b=h.newHandle(256);
    assert(a&&b);compare(h);assert(h.setHandleSize(a,128)==0);compare(h);
    // CompactUp can create a free block before the previously occupied prefix.
    assert(h.init(arena,sizeof arena));a=h.newHandle(1024);b=h.newHandle(256);
    assert(a&&b);compare(h);h.compactUp();compare(h);
    assert(h.init(arena,sizeof arena));compare(h);
    assert(h.init(arena,sizeof arena,2));MacHeap::Handle handles[48]={};uint32_t rng=1;
    for(unsigned step=0;step<2500;++step) {
        rng=rng*1664525u+1013904223u;unsigned slot=(rng>>16)%48;
        if(!handles[slot])handles[slot]=h.newHandle(rng%900,true);
        else if(step%3)h.setHandleSize(handles[slot],rng%2048);
        else {assert(h.disposeHandle(handles[slot])==0);handles[slot]=nullptr;}
        if(step%7==0)h.compact();
        if(step%11==0 && handles[slot])h.moveHigh(handles[slot]);
        if(step%17==0){auto p=h.newPtr(25);if(p)h.disposePtr(p);}
        if(step%19==0)h.moreMasters(2);
        compare(h);
    }
    // Mutation control: a stale interior boundary and an occupied-prefix lie
    // both fail the independent whole-zone invariant.
    auto first=h.searchStart_;h.searchStart_=first+8;assert(!h.check());h.searchStart_=first;
    h.searchStart_=h.end_;assert(!h.check());h.searchStart_=first;assert(h.check());
    std::puts("HEAP SEARCH ADAPTER PASS: 2500 mixed operations, 20040 first-fit comparisons, reset and two stale-hint controls");
}
