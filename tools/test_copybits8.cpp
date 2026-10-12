#include "../src/mac/CopyBits8.h"
#include <cassert>
#include <vector>
#include <fstream>
#include <iterator>
#include <string>
#include <cstdio>
static std::vector<uint8_t> box(int t,int l,int b,int r) {
    std::vector<uint8_t> out;for(int v:{t,l,b,r}) { out.push_back(uint16_t(v)>>8);out.push_back(uint8_t(v)); }return out;
}
static std::vector<uint8_t> read(const std::string& path) {
    std::ifstream in(path,std::ios::binary);assert(in.good());return {std::istreambuf_iterator<char>(in),{}};
}
static void testMaskCache() {
    static RegionRows::ValidationCache cache;
    const int16_t words[]={42,0,-2,8,12,0,-2,3,8,12,32767,4,3,8,32767,8,-2,12,32767,32767};
    std::vector<uint8_t> original;
    for(int16_t v:words){original.push_back(uint16_t(v)>>8);original.push_back(uint8_t(v));}
    original[1]=uint8_t(original.size());
    auto map=box(-10,-12,30,38),from=box(-2,-4,12,16),scaled=box(-2,-4,26,36);
    std::vector<uint8_t> source(56*40),initial(source.size(),0xa5);
    for(unsigned i=0;i<source.size();++i)source[i]=uint8_t(i*37+13);
    auto check=[&](const std::vector<uint8_t>& region,uint16_t capacity) {
        for(bool scale:{false,true}) {
            auto expected=initial,actual=initial;uint8_t a[8]={},b[8]={};
            const auto& to=scale?scaled:from;
            bool reference=CopyBits8::copy(source.data(),source.size(),56,map.data(),expected.data(),expected.size(),56,map.data(),from.data(),to.data(),map.data(),map.data(),map.data(),a,nullptr,region.data(),capacity);
            bool cached=CopyBits8::copy(source.data(),source.size(),56,map.data(),actual.data(),actual.size(),56,map.data(),from.data(),to.data(),map.data(),map.data(),map.data(),b,nullptr,region.data(),capacity,&cache);
            assert(cached==reference && actual==expected);
            if(cached)for(unsigned i=0;i<8;++i)assert(a[i]==b[i]);
            else assert(actual==initial); // Malformed tails cannot partially draw.
        }
    };
    check(original,original.size());check(original,original.size());
    for(unsigned i=0;i<original.size();++i)for(uint8_t delta:{uint8_t(1),uint8_t(255)}) {
        auto changed=original;changed[i]^=delta;
        check(changed,changed.size());check(original,original.size());
    }
    // Warm content moved into new storage, truncated capacity, and slot reuse.
    auto moved=original;check(moved,moved.size());check(moved,moved.size()-2);
    for(unsigned i=0;i<20;++i) {auto changed=original;changed[5]=uint8_t(i);check(changed,changed.size());}
    check(original,original.size());
    // Valid but oversized streams take the uncached path; a bad final marker
    // must be rejected there too. Empty transitions still require validation.
    std::vector<uint8_t> large(10,0);
    for(unsigned y=0;y<1100;++y)for(uint16_t v:{uint16_t(y),uint16_t(32767)}) {
        large.push_back(v>>8);large.push_back(v);
    }
    large.push_back(127);large.push_back(255);
    large[0]=large.size()>>8;large[1]=large.size();
    check(large,large.size());large.back()=0;check(large,large.size());
    puts("PASS mask cache: exact-content reuse, every-byte mutation, relocation, capacity, eviction, scaling and atomic rejection");
}
int main(int argc,char** argv) {
    testMaskCache();
    // Exercise all longword alignments and tails, retaining sentinels on both
    // sides. This also checks zero-length copies without touching either side.
    for(unsigned si=0;si<4;++si)for(unsigned di=0;di<4;++di)
        for(unsigned count=0;count<=129;++count) {
            std::vector<uint8_t> source(140),actual(140,0xa5),expected=actual;
            for(unsigned i=0;i<source.size();++i)source[i]=uint8_t(i*37+13);
            for(unsigned i=0;i<count;++i)expected[di+4+i]=source[si+4+i];
            CopyBits8::copySpan(source.data()+si+4,actual.data()+di+4,count);
            assert(actual==expected);
        }
    auto sm=box(-5,-7,25,33),dm=box(-10,-12,30,38),vis=box(-8,-9,28,35),clip=box(-3,-4,22,30);
    std::vector<uint8_t> src(44*30),initial(56*40,0xa5);for(unsigned i=0;i<src.size();++i)src[i]=uint8_t(i*37+13);
    uint8_t colors[256];for(unsigned i=0;i<256;++i)colors[i]=uint8_t(255-i);
    for(bool mapped:{false,true})for(int offset=-30;offset<31;++offset) {
        auto from=box(-9,-10,21,30),to=box(offset,offset-4,offset+30,offset+36);auto expected=initial,actual=initial;uint8_t drawn[8];
        for(int y=-10;y<30;++y)for(int x=-12;x<38;++x) {
            int sy=-9+y-offset,sx=-10+x-(offset-4);
            if(y>=offset && y<offset+30 && x>=offset-4 && x<offset+36
               && y>=-8 && y<28 && x>=-9 && x<35 && y>=-3 && y<22 && x>=-4 && x<30
               && sy>=-5 && sy<25 && sx>=-7 && sx<33)expected[(y+10)*56+x+12]=mapped ? 255-src[(sy+5)*44+sx+7] : src[(sy+5)*44+sx+7];
        }
        assert(CopyBits8::copy(src.data(),src.size(),44,sm.data(),actual.data(),actual.size(),56,dm.data(),from.data(),to.data(),dm.data(),vis.data(),clip.data(),drawn,mapped?colors:nullptr));
        assert(actual==expected);
    }
    // Irregular XOR region: two separated spans then a merged span. Verify
    // every byte, including row padding, clipped pixels and colour remapping.
    const int16_t words[]={44,0,-2,8,12,0,-2,3,8,12,32767,4,3,8,32767,8,-2,12,32767,32767};
    std::vector<uint8_t> mask;for(int16_t v:words){mask.push_back(uint16_t(v)>>8);mask.push_back(uint8_t(v));}
    mask[1]=uint8_t(mask.size());
    for(bool mapped:{false,true}) {
        auto r=box(-2,-4,12,16),dst=initial,expected=initial;uint8_t bounds[8];
        for(int y=0;y<8;++y)for(int x=-2;x<12;++x)if(y>=4 || x<3 || x>=8)
            expected[(y+10)*56+x+12]=mapped?255-src[(y+5)*44+x+7]:src[(y+5)*44+x+7];
        assert(CopyBits8::copy(src.data(),src.size(),44,sm.data(),dst.data(),dst.size(),56,dm.data(),r.data(),r.data(),dm.data(),vis.data(),clip.data(),bounds,mapped?colors:nullptr,mask.data(),mask.size()));
        assert(dst==expected);
        dst=initial;mask.back()=0;
        assert(!CopyBits8::copy(src.data(),src.size(),44,sm.data(),dst.data(),dst.size(),56,dm.data(),r.data(),r.data(),dm.data(),vis.data(),clip.data(),bounds,nullptr,mask.data(),mask.size()));
        assert(dst==initial);mask.back()=255;
    }
    // The same irregular mask against rectangles on each side, inside it and
    // crossing its edges. Expected membership is independent of region decoding.
    for(int t=-6;t<=12;t+=3)for(int l=-8;l<=16;l+=3) {
        auto r=box(t,l,t+5,l+7),dst=initial,expected=initial;uint8_t bounds[8];
        for(int y=0;y<8;++y)for(int x=-2;x<12;++x)
            if(y>=t && y<t+5 && x>=l && x<l+7 && (y>=4 || x<3 || x>=8))
                expected[(y+10)*56+x+12]=src[(y+5)*44+x+7];
        assert(CopyBits8::copy(src.data(),src.size(),44,sm.data(),dst.data(),dst.size(),56,dm.data(),r.data(),r.data(),dm.data(),vis.data(),clip.data(),bounds,nullptr,mask.data(),mask.size()));
        assert(dst==expected);
        if(t>=8 || t+5<=0 || l>=12 || l+7<=-2)
            for(uint8_t v:bounds)assert(v==0);
    }
    // An entirely disjoint malformed mask must still fail without writes.
    {
        auto r=box(10,20,12,24),dst=initial;uint8_t bounds[8];mask.back()=0;
        assert(!CopyBits8::copy(src.data(),src.size(),44,sm.data(),dst.data(),dst.size(),56,dm.data(),r.data(),r.data(),dm.data(),vis.data(),clip.data(),bounds,nullptr,mask.data(),mask.size()));
        assert(dst==initial);mask.back()=255;
    }
    auto from=box(0,0,10,10),to=box(0,0,11,10);auto actual=initial;uint8_t drawn[8];
    assert(CopyBits8::copy(src.data(),src.size(),44,sm.data(),actual.data(),actual.size(),56,dm.data(),from.data(),to.data(),dm.data(),vis.data(),clip.data(),drawn));
    actual=initial;
    assert(!CopyBits8::copy(src.data(),1,44,sm.data(),actual.data(),actual.size(),56,dm.data(),from.data(),from.data(),dm.data(),vis.data(),clip.data(),drawn));assert(actual==initial);
    if(argc==3) {
        std::string p=std::string(argv[1])+"/maskcopy-reference-"+argv[2];
        auto sp=read(p+"enter-src-pm.bin"),dp=read(p+"enter-dst-pm.bin");
        auto source=read(p+"enter-src-pixels.bin"),dst=read(p+"enter-dst-pixels.bin"),expected=read(p+"return-dst-pixels.bin");
        auto port=read(p+"enter-port.bin"),v=read(p+"enter-vis.bin"),c=read(p+"enter-clip.bin"),mask=read(p+"enter-mask.bin"),r=read(p+"enter-from.bin");
        assert(CopyBits8::copy(source.data(),source.size(),RectBounds::word(sp.data()+4)&0x3fff,sp.data()+6,dst.data(),dst.size(),RectBounds::word(dp.data()+4)&0x3fff,dp.data()+6,r.data(),r.data(),port.data()+16,v.data()+2,c.data()+2,drawn,nullptr,mask.data(),mask.size()));
        assert(dst==expected);
        puts("PASS exact original masked-copy destination");
    }
    if(argc==2) {
        std::string p=std::string(argv[1])+"/copy8-reference-";
        auto sp=read(p+"enter-src-pm.bin"),dp=read(p+"enter-dst-pm.bin");
        auto pixels=read(p+"enter-src-pixels.bin"),dst=read(p+"enter-dst-pixels.bin"),expected=read(p+"return-dst-pixels.bin");
        auto port=read(p+"enter-port.bin"),v=read(p+"enter-vis.bin"),c=read(p+"enter-clip.bin"),r=box(0,0,200,320);
        assert(CopyBits8::copy(pixels.data(),pixels.size(),RectBounds::word(sp.data()+4)&0x3fff,sp.data()+6,dst.data(),dst.size(),RectBounds::word(dp.data()+4)&0x3fff,dp.data()+6,r.data(),r.data(),port.data()+16,v.data()+2,c.data()+2,drawn));
        assert(dst==expected);
    }
    puts("PASS CopyBits8: 122 full-buffer clipped direct/remapped copies, irregular masks and atomic malformed-mask rejection, scaling and rejected capacity mutation, optional original Mac capture");
}
