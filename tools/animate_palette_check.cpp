#include <cstdint>
#include <fstream>
#include <vector>
#include <cassert>
#include "mac/AnimatePalette8.h"
static std::vector<uint8_t> read(const char* name) {
    std::ifstream f(name,std::ios::binary);assert(f.good());
    return {std::istreambuf_iterator<char>(f),{}};
}
int main(int argc,char** argv) {
    assert(argc==8);auto p=read(argv[1]),c=read(argv[2]),s=read(argv[3]),a=read(argv[4]);
    auto afterP=read(argv[5]),afterC=read(argv[6]);bool active=argv[7][0]=='1';
    assert(a.size()>=14);
    // Wrong source flags, range and palette usage must reject before mutation.
    for(unsigned control=0;control<3;++control) {
        auto badP=p,badC=c,badS=s;
        unsigned first=Palette8::word(a.data()+4),dest=Palette8::word(a.data()+2),count=Palette8::word(a.data());
        if(control==0)badS[4]=0x80;
        if(control==1)count=256;
        if(control==2)badP[16+16+7]=10;
        auto beforeP=badP,beforeC=badC;
        assert(!AnimatePalette8::animate(badP.data(),badP.size(),badC.data(),badC.size(),badS.data(),badS.size(),first,dest,count,active));
        assert(badP==beforeP && badC==beforeC);
    }
    assert(AnimatePalette8::animate(p.data(),p.size(),c.data(),c.size(),s.data(),s.size(),
        Palette8::word(a.data()+4),Palette8::word(a.data()+2),Palette8::word(a.data()),active));
    assert(p==afterP && c==afterC);
}
