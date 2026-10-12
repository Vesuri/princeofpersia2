#include <cassert>
#include <cstdio>
#include <vector>
#include <fstream>
#include <iterator>
#include <cstring>
#include "../src/mac/ResourceMap.h"
static void w(std::vector<uint8_t>& b,unsigned o,unsigned v) { b[o]=v>>8;b[o+1]=v; }
static void l(std::vector<uint8_t>& b,unsigned o,uint32_t v) { w(b,o,v>>16);w(b,o+2,v); }
static uint32_t get(const uint8_t* b) { return (uint32_t)b[0]<<24|(uint32_t)b[1]<<16|(uint32_t)b[2]<<8|b[3]; }
int main(int argc,char** argv) {
    std::vector<uint8_t> h(16),m(72);l(h,0,256);l(h,4,1024);l(h,8,64);l(h,12,m.size());
    std::copy(h.begin(),h.end(),m.begin());w(m,24,28);w(m,26,70);w(m,28,1);
    l(m,30,0x54455354);w(m,34,0);w(m,36,18); // TEST, one resource, reference at 46
    l(m,38,0x434f4445);w(m,42,0);w(m,44,30); // CODE, reference at 58 (preserve order)
    w(m,46,0xfffe);w(m,48,0);m[50]=0x60;m[53]=8;
    w(m,58,128);w(m,60,0xffff);m[65]=20;m[70]=1;m[71]='x';
    ResourceMap map;ResourceMap::Entry e;ResourceMap::Layout layout;uint32_t offset=0;
    assert(ResourceMap::layout(h.data(),1096,layout));assert(layout.mapLength==72);
    assert(map.open(h.data(),1096,m.data(),m.size()) && map.count()==2);
    assert(map.entry(0,e) && e.type==0x54455354 && e.id==-2 && e.attrs==0x60 && e.nameLength==1 && *e.name=='x' && e.lengthOffset==264);
    assert(map.payload(0,52,offset) && offset==268);assert(!map.payload(0,53,offset));assert(!map.payload(0,0xffffffff,offset));
    assert(map.entry(1,e) && e.type==0x434f4445 && e.id==128 && !e.name && e.lengthOffset==276);
    assert(map.payload(1,0,offset) && offset==280);assert(!map.entry(2,e));
    // No resource payload buffer is provided. These checks cannot accidentally
    // depend on preloaded data or trust a resource length before its own read.
    for(unsigned size=0;size<m.size();++size)assert(!map.open(h.data(),1096,m.data(),size) && !map.count());
    for(unsigned mutation=0;mutation<10;++mutation) {
        auto bad=m;
        switch(mutation) {
        case 0:bad[0]^=1;break;case 1:w(bad,24,71);break;case 2:w(bad,26,71);bad[71]=255;break;
        case 3:w(bad,28,768);break;case 4:w(bad,34,0xffff);break;case 5:w(bad,36,0);break;
        case 6:w(bad,44,18);break;case 7:l(bad,38,0x54455354);break;case 8:bad[53]=63;break;
        case 9:w(bad,48,32767);break;
        }
        assert(!map.open(h.data(),1096,bad.data(),bad.size()) && !map.count());
    }
    auto duplicate=m;w(duplicate,28,0);w(duplicate,34,1);w(duplicate,58,0xfffe);
    assert(map.open(h.data(),1096,duplicate.data(),duplicate.size()) && map.count()==2);
    assert(map.entry(0,e) && e.type==0x54455354 && e.id==-2 && e.lengthOffset==264 && e.nameLength==1);
    assert(map.entry(1,e) && e.type==0x54455354 && e.id==-2 && e.lengthOffset==276 && !e.name);
    auto emptyH=h;std::vector<uint8_t> empty(30);l(emptyH,8,0);l(emptyH,12,30);
    std::copy(emptyH.begin(),emptyH.end(),empty.begin());w(empty,24,28);w(empty,26,30);w(empty,28,65535);
    assert(map.open(emptyH.data(),1054,empty.data(),30) && !map.count() && !map.entry(0,e));
    auto badH=h;l(badH,4,260);assert(!ResourceMap::layout(badH.data(),1096,layout));
    l(badH,4,0xfffffff0);assert(!ResourceMap::layout(badH.data(),0xffffffff,layout));
    puts("PASS resource-map: map-only parsing, original order, empty map, names/IDs, deferred lengths, malformed ranges");
    if(argc==2) {
        std::ifstream input(argv[1],std::ios::binary);assert(input.good());
        std::vector<uint8_t> raw((std::istreambuf_iterator<char>(input)),{});
        assert(ResourceMap::layout(raw.data(),raw.size(),layout));
        std::vector<uint8_t> header(raw.begin(),raw.begin()+16);
        std::vector<uint8_t> mapBytes(raw.begin()+layout.mapOffset,raw.begin()+layout.mapOffset+layout.mapLength);
        assert(map.open(header.data(),raw.size(),mapBytes.data(),mapBytes.size()));
        for(unsigned i=0;i<map.count();++i) {
            assert(map.entry(i,e));uint32_t size=get(raw.data()+e.lengthOffset);
            assert(map.payload(i,size,offset));uint32_t hash=2166136261u;
            for(uint32_t j=0;j<size;++j)hash=(hash^raw[offset+j])*16777619u;
            printf("RESOURCE %08x %d %u %u %08x ",e.type,e.id,e.attrs,size,hash);
            for(unsigned j=0;j<e.nameLength;++j)printf("%02x",e.name[j]);
            puts("");
        }
        printf("PASS original resource map: resources=%u map-bytes=%u fork-bytes=%zu\n",map.count(),layout.mapLength,raw.size());
    }
}
