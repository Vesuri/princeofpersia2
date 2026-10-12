#include <cstdint>
#include <cassert>
#include <cstdio>
#include <vector>
#include <array>
#include "../src/mac/ColorMap8Cache.h"
using Table=std::array<uint8_t,2056>;
static void palette(Table& t,uint32_t seed,bool reversed) {
    GWorld8::longword(t.data(),seed);GWorld8::word(t.data()+4,0);GWorld8::word(t.data()+6,255);
    for(unsigned i=0;i<256;++i) {
        GWorld8::word(t.data()+8+8*i,i);
        GWorld8::word(t.data()+10+8*i,uint16_t((reversed?255-i:i)*257));
        GWorld8::word(t.data()+12+8*i,0);GWorld8::word(t.data()+14+8*i,0);
    }
}
int main() {
    Table source{},destination{};palette(source,1,true);palette(destination,2,false);
    std::vector<uint16_t> grid(34*34*34),queue(32*32*32);
    std::vector<uint8_t> inverse(32768+524);
    auto build=[&](unsigned res) {assert(GWorld8::inverse(destination.data(),res,inverse.data(),grid.data(),queue.data()));};
    ColorMap8Cache cache;const uint8_t* colors;bool hit;
    auto map=[&]() {return cache.map(source.data(),destination.data(),inverse.data(),colors,hit);};
    auto check=[&](bool reverse) {for(unsigned i=0;i<256;++i)assert(colors[i]==(reverse?255-i:i));};
    build(4);assert(map()&&!hit);check(true);
    const uint8_t* owned=colors;assert(map()&&hit&&colors==owned);check(true);
    palette(source,3,false);assert(map()&&!hit);check(false);
    palette(destination,4,true);build(4);assert(map()&&!hit);check(true);
    build(5);assert(map()&&!hit);check(true);
    GWorld8::word(source.data()+6,254);assert(!map()&&colors==nullptr&&!hit);
    GWorld8::word(source.data()+6,255);GWorld8::word(source.data()+4,1);assert(!map());
    GWorld8::word(source.data()+4,0);GWorld8::longword(source.data(),5);
    GWorld8::word(source.data()+8,9);assert(!map()&&colors==nullptr);
    GWorld8::word(source.data()+8,0);assert(map()&&!hit);check(true);
    GWorld8::longword(inverse.data(),6);assert(!map());build(5);
    std::array<Table,5> relocated;
    for(auto& t:relocated) {t=source;assert(cache.map(t.data(),destination.data(),inverse.data(),colors,hit)&&!hit);check(true);}
    assert(cache.map(relocated[0].data(),destination.data(),inverse.data(),colors,hit)&&!hit);
    assert(cache.map(relocated[0].data(),destination.data(),inverse.data(),colors,hit)&&hit);
    std::puts("PASS colormap8 cache: exact red-channel permutation, source/destination seeds, resolution, relocation, eviction, malformed inputs and failed-build retry");
}
