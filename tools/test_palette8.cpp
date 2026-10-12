#include "../src/mac/Palette8.h"
#include <cassert>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#include <cstdio>
#include <iostream>
static std::vector<uint8_t> read(const std::string& p) {
    std::ifstream f(p,std::ios::binary);assert(f.good());return {std::istreambuf_iterator<char>(f),{}};
}
static void synthetic() {
    std::vector<uint8_t> palette(4112),table(2056),priv(4);
    Palette8::word(palette.data(),256);
    for(unsigned i=0;i<256;++i) {
        auto e=palette.data()+16+i*16;
        Palette8::word(e,i*257);Palette8::word(e+2,(255-i)*257);Palette8::word(e+4,0x1234);
        Palette8::word(e+6,10);
    }
    for(unsigned i:{0u,63u})for(unsigned c=0;c<3;++c)Palette8::word(palette.data()+16+i*16+c*2,65535);
    for(unsigned i:{62u,255u})for(unsigned c=0;c<3;++c)Palette8::word(palette.data()+16+i*16+c*2,0);
    Palette8::systemTable(table.data(),17);auto initial=table;auto source=palette;
    assert(Palette8::realize(palette.data(),4112,table.data(),2056,priv.data(),4,23));
    for(unsigned i=0;i<256;++i) {
        const auto expected=(i==0 || i==63 || i==62 || i==255) ? initial.data()+10+i*8 : source.data()+16+i*16;
        for(unsigned c=0;c<6;++c)assert(table[10+i*8+c]==expected[c]);
        assert(Palette8::word(palette.data()+26+i*16)==0x800a);
    }
    assert(Palette8::word(table.data()+2)==23 && Palette8::word(priv.data()+2)==23);
    auto realized=table;auto privateRealized=priv;auto done=palette;
    assert(!Palette8::realize(palette.data(),4112,table.data(),2056,priv.data(),4,24));
    assert(table==realized && priv==privateRealized && palette==done);
    assert(!Palette8::realize(palette.data(),4112,table.data(),2056,priv.data(),4,24,1));
    assert(table==realized && priv==privateRealized && palette==done);
    assert(Palette8::realize(palette.data(),4112,table.data(),2056,priv.data(),4,24,0x800a));
    Palette8::longword(realized.data(),24);Palette8::longword(privateRealized.data(),24);
    assert(table==realized && priv==privateRealized && palette==done);
    for(unsigned kind=0;kind<8;++kind) {
        palette=source;table=initial;priv.assign(4,0);
        unsigned pb=4112,tb=2056,xb=4;
        if(kind==0)pb--;
        if(kind==1)tb--;
        if(kind==2)xb--;
        if(kind==3)palette[0]=0;
        if(kind==4)palette[16+254*16+7]=11;
        if(kind==5)palette[16+253*16+11]=1;
        if(kind==6)table[10]=0;
        if(kind==7)palette[16]=0;
        auto a=palette,b=table,c=priv;
        assert(!Palette8::realize(palette.data(),pb,table.data(),tb,priv.data(),xb,24));
        assert(palette==a && table==b && priv==c);
    }
}
int main(int argc,char** argv) {
    synthetic();
    if(argc==1) { puts("PASS Palette8 synthetic endpoint retention, malformed-state rejection and mutation atomicity");return 0; }
    if(argc==3 && (std::string(argv[1])=="--replacement" || std::string(argv[1])=="--rebind")) {
        std::string root=argv[2];
        bool rebind=std::string(argv[1])=="--rebind";
        std::string prefix=root+(rebind ? "/rebind-reference-" : "/binding129-reference-");
        auto palette=read(prefix+"enter-palette.bin");
        auto expected=read(prefix+"return-palette.bin");
        auto table=read(prefix+"enter-clut.bin");
        auto after=read(prefix+"return-clut.bin");
        auto priv=read(prefix+"enter-private.bin");
        auto privateAfter=read(prefix+"return-private.bin");
        uint32_t seed=uint32_t(Palette8::word(after.data()))<<16|Palette8::word(after.data()+2);
        assert(Palette8::realize(palette.data(),palette.size(),table.data(),table.size(),priv.data(),priv.size(),seed,rebind ? 0x800a : 0,rebind));
        Palette8::longword(palette.data()+4,0xc003);Palette8::longword(palette.data()+8,1);
        assert(palette==expected && table==after && priv==privateAfter);
        puts("PASS Palette8 replacement: complete palette, device table and private seed match Mac");return 0;
    }
    if(argc==3 && std::string(argv[1])=="--restore") {
        std::string root=argv[2];
        auto palette=read(root+"/restorepal-reference-enter-palette.bin");
        auto expected=read(root+"/restorepal-reference-return-palette.bin");
        auto table=read(root+"/restorepal-reference-enter-clut.bin");
        auto after=read(root+"/restorepal-reference-return-clut.bin");
        auto priv=read(root+"/restorepal-reference-enter-private.bin");
        auto privateAfter=read(root+"/restorepal-reference-return-private.bin");
        uint32_t seed=uint32_t(Palette8::word(after.data()))<<16|Palette8::word(after.data()+2);
        assert(Palette8::realize(palette.data(),palette.size(),table.data(),table.size(),priv.data(),priv.size(),seed,0x800a));
        assert(palette==expected && table==after && priv==privateAfter);
        puts("PASS Palette8 restore: complete palette, device table and private seed match Mac");return 0;
    }
    assert(argc==2);
    if(std::string(argv[1])=="--system-table") {
        std::vector<uint8_t> table(2056);Palette8::systemTable(table.data(),0);
        std::cout.write((const char*)table.data(),table.size());return 0;
    }
    std::string root=argv[1];
    auto before=read(root+"/windowstate-reference-show-before-clut.bin");
    auto after=read(root+"/windowstate-reference-show-after-clut.bin");
    auto palette=read(root+"/windowstate-reference-show-before-palette.bin");
    auto expected=read(root+"/windowstate-reference-show-after-palette.bin");
    auto privateData=read(root+"/windowstate-reference-show-before-private.bin");
    auto privateAfter=read(root+"/windowstate-reference-show-after-private.bin");
    assert(before.size()==2056 && after.size()==2056 && privateAfter.size()==4);
    uint32_t oldSeed=uint32_t(Palette8::word(before.data()))<<16|Palette8::word(before.data()+2);
    uint32_t newSeed=uint32_t(Palette8::word(after.data()))<<16|Palette8::word(after.data()+2);
    std::vector<uint8_t> initial(2056);Palette8::systemTable(initial.data(),oldSeed);assert(initial==before);
    assert(Palette8::realize(palette.data(),palette.size(),initial.data(),initial.size(),privateData.data(),privateData.size(),newSeed));
    assert(initial==after && palette==expected && privateData==privateAfter);
    puts("PASS native Palette8 helper: exact original 256-entry system table and complete ShowWindow palette/CLUT/private transition");
}
