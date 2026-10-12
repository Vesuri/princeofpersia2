#include <cstdint>
#include <fstream>
#include <vector>
#include <cassert>
#include <cstdio>
#include "mac/CopyBits8.h"
static std::vector<uint8_t> read(const char* name) {
    std::ifstream f(name,std::ios::binary);assert(f.good());
    return {std::istreambuf_iterator<char>(f),{}};
}
int main(int argc,char** argv) {
    assert(argc==12);
    auto src=read(argv[1]),dst=read(argv[2]),sm=read(argv[3]),dm=read(argv[4]);
    auto from=read(argv[5]),to=read(argv[6]),port=read(argv[7]),vis=read(argv[8]),clip=read(argv[9]);
    auto expected=read(argv[10]),args=read(argv[11]);
    assert(sm.size()==50 && dm.size()==50 && from.size()==8 && to.size()==8 && port.size()==108 && vis.size()==8 && clip.size()==8 && args.size()>=22);
    for(unsigned i=0;i<6;++i)assert(args[i]==0); // srcCopy, no mask.
    uint8_t drawn[8];
    assert(CopyBits8::copy(src.data(),src.size(),RectBounds::word(sm.data()+4)&0x3fff,sm.data()+6,
        dst.data(),dst.size(),RectBounds::word(dm.data()+4)&0x3fff,dm.data()+6,
        from.data(),to.data(),port.data()+16,vis.data(),clip.data(),drawn));
    for(unsigned i=0,n=0;i<dst.size() && n<12;++i)if(dst[i]!=expected[i]) {fprintf(stderr,"pixel %u: %u expected %u\n",i,dst[i],expected[i]);++n;}
    assert(dst==expected); // Also checks every byte outside the copied rectangle.
}
