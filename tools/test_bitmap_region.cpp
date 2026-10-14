#include <cassert>
#include <cstdint>
#include <cstring>
#include "mac/BitmapRegion.h"
int main(){
    uint8_t bounds[8]={0,3,0,5,0,6,0,13},output[1024];uint16_t size=0;
    uint32_t random=17;
    for(unsigned test=0;test<1000;++test){
        uint8_t pixels[6];for(auto& p:pixels){random=random*1664525+1013904223;p=random>>24;}
        assert(BitmapRegion::encode(pixels,6,2,bounds,output,sizeof output,size));
        for(int y=3;y<6;++y){
            RegionRows::Edges edges{};assert(RegionRows::row(output,size,y,edges));
            for(int x=5;x<13;++x)assert(RegionRows::inside(edges,x)==bool(pixels[(y-3)*2]&(128>>(x-5))));
        }
    }
    uint8_t clear[6]={};assert(BitmapRegion::encode(clear,6,2,bounds,output,sizeof output,size));
    assert(size==10);for(unsigned i=2;i<10;++i)assert(output[i]==0);
    uint8_t full[6]={255,0,255,0,255,0};assert(BitmapRegion::encode(full,6,2,bounds,output,sizeof output,size));
    assert(size==10 && !std::memcmp(output+2,bounds,8));
    // Two individually small edge sets can temporarily exceed sixteen while
    // their XOR transitions are applied in sorted order.
    RegionOps::Writer writer(output,sizeof output);RegionRows::Edges a{},b{},empty{};
    for(int x=16;x<32;++x)assert(RegionRows::toggle(a,x));
    for(int x=0;x<16;++x)assert(RegionRows::toggle(b,x));
    assert(writer.row(0,a) && writer.row(1,b) && writer.row(2,empty) && writer.finish(size));
    RegionRows::Edges decoded{};assert(RegionRows::row(output,size,1,decoded));
    assert(decoded.count==b.count);for(unsigned i=0;i<b.count;++i)assert(decoded.x[i]==b.x[i]);
    assert(!BitmapRegion::encode(full,5,2,bounds,output,sizeof output,size));
    assert(!BitmapRegion::encode(full,6,2,bounds,output,9,size));
}
