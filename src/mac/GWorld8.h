#ifndef AMIGAXDEV_GWORLD8_H
#define AMIGAXDEV_GWORLD8_H
#ifndef __mc68000__
#include <stdint.h>
#endif
namespace GWorld8 {
struct Rect { int16_t top,left,bottom,right; };
struct Layout { uint16_t rowBytes; uint32_t pixelBytes; };
inline bool layout(const Rect& r,Layout& out) {
    int32_t width=int32_t(r.right)-r.left,height=int32_t(r.bottom)-r.top;
    if(width<=0 || height<=0)return false;
    // Measured original QDOffscreen allocation keeps an extra 32-bit slop word.
    uint32_t stride=((uint32_t(width)+3)&~3UL)+4;
    if(stride>=0x4000)return false;
    out={uint16_t(stride),stride*uint32_t(height)};return true;
}
struct PixelAddress { uint32_t result,d0,a0,a1; };
// System 7.5.5 uses different scratch-register results for handle/raw bases.
// Ownership and lock state are supplied by the runtime, never inferred from an
// arbitrary caller pointer. Preserve all 32 bits of the Amiga pixel address.
inline bool pixelAddress(uint16_t version,uint32_t base,uint32_t pixelHandle,
                         uint32_t pixels,uint32_t map,bool locked,uint32_t d0,
                         PixelAddress& out) {
    if(!pixelHandle || !pixels || !map)return false;
    if(locked && version==1 && base==pixels) {
        out={pixels,uint32_t((d0&0xffff0000UL)|1),pixels,map};return true;
    }
    if(!locked && version==2 && base==pixelHandle) {
        out={pixels,pixels,pixelHandle,map};return true;
    }
    return false;
}
inline void word(uint8_t* p,uint16_t v) { p[0]=uint8_t(v>>8);p[1]=uint8_t(v); }
inline void longword(uint8_t* p,uint32_t v) { word(p,uint16_t(v>>16));word(p+2,uint16_t(v)); }
inline void rect(uint8_t* p,const Rect& r) { word(p,uint16_t(r.top));word(p+2,uint16_t(r.left));word(p+4,uint16_t(r.bottom));word(p+6,uint16_t(r.right)); }
inline void clear(uint8_t* p,uint16_t n) { for(uint16_t i=0;i<n;++i)p[i]=0; }
inline void pixmap(uint8_t* p,uint32_t pixels,uint32_t colors,const Rect& r,const Layout& l) {
    clear(p,50);longword(p,pixels);word(p+4,uint16_t(0x8000|l.rowBytes));rect(p+6,r);
    word(p+14,2);longword(p+22,72UL<<16);longword(p+26,72UL<<16);
    word(p+32,8);word(p+34,1);word(p+36,8);longword(p+42,colors);
}
inline void port(uint8_t* p,uint32_t pm,uint32_t vars,uint32_t vis,uint32_t clip,
                 uint32_t back,uint32_t pen,uint32_t fill,const Rect& r) {
    clear(p,108);longword(p+2,pm);word(p+6,0xc001);longword(p+8,vars);word(p+14,0x8000);
    rect(p+16,r);longword(p+24,vis);longword(p+28,clip);longword(p+32,back);
    word(p+42,0xffff);word(p+44,0xffff);word(p+46,0xffff);
    word(p+52,1);word(p+54,1);word(p+56,8);longword(p+58,pen);longword(p+62,fill);
    word(p+72,1);longword(p+80,255);
}
inline uint16_t readword(const uint8_t* p) {
#ifdef __mc68000__
    struct __attribute__((packed, may_alias)) Word { uint16_t value; };
    uint16_t value;
    __asm__("move.w %1,%0" : "=d"(value) : "m"(*(const Word*)p) : "cc");
    return value;
#else
    return uint16_t(uint16_t(p[0])<<8|p[1]);
#endif
}
// The inverse cube picks a collision ring. System 7.5.5 refines that
// ring by RGB16 Manhattan distance; equal distances retain the first entry.
inline bool colorIndex(const uint8_t* colors,const uint8_t* inverseTable,
                       const uint8_t* rgb,uint16_t& result) {
    if(!colors || !inverseTable || !rgb || readword(colors+6)!=255
       || (readword(colors+4)!=0 && readword(colors+4)!=0x8000))return false;
    for(unsigned i=0;i<4;++i)if(colors[i]!=inverseTable[i])return false;
    uint16_t resolution=readword(inverseTable+4);
    if(resolution!=4 && resolution!=5)return false;
    uint16_t wanted[3]={readword(rgb),readword(rgb+2),readword(rgb+4)};
    uint32_t cell=0;
    for(unsigned i=0;i<3;++i)cell=(cell<<resolution)|(wanted[i]>>(16-resolution));
    uint32_t cube=1UL<<(3*resolution);
    const uint8_t* links=inverseTable+12+cube;
    uint16_t first=inverseTable[6+cell],current=first,best=first;
    uint32_t distance=0xffffffffUL;
    for(unsigned visited=0;visited<256;++visited) {
        const uint8_t* entry=colors+8+8*current;
        if(readword(entry)&0x4000)return false;
        if(readword(colors+4)==0 && (readword(entry)&255)!=current)return false;
        uint32_t d=0;
        for(unsigned i=0;i<3;++i) {
            int32_t delta=int32_t(wanted[i])-readword(entry+2+2*i);
            d+=uint32_t(delta<0?-delta:delta);
        }
        if(d<distance) {distance=d;best=current;}
        current=links[current];
        if(current==first) {result=best;return true;}
    }
    return false; // Malformed collision cycle; never publish a guessed colour.
}
// Measured MakeITable: seed quantized cells, preserve collision rings, then
// breadth-first fill with alternating neighbour order. Tail scratch is cleared;
// only the header, lookup cube and 262-byte collision record are defined.
inline bool inverse(const uint8_t* colors,uint16_t resolution,uint8_t* out,
                    uint16_t* grid,uint16_t* queue) {
    if(!colors || !out || !grid || !queue || (resolution!=4 && resolution!=5)
       || readword(colors+6)!=255)return false;
    bool colored=false;
    for(uint16_t i=0;i<256;++i) {
        const uint8_t* c=colors+8+i*8;
        if(!(readword(c)&0x4000) && (readword(c+2)!=readword(c+4) || readword(c+4)!=readword(c+6)))colored=true;
    }
    if(!colored)return false; // The original uses a distinct grey-only builder.
    uint32_t n=1UL<<resolution,stride=n+2,cube=n*n*n,padded=stride*stride*stride;
    clear(out,uint16_t(cube+524));
    for(uint32_t i=0;i<padded;++i)grid[i]=0x7fff;
    for(uint32_t r=1;r<=n;++r)for(uint32_t g=1;g<=n;++g)for(uint32_t b=1;b<=n;++b)
        grid[r*stride*stride+g*stride+b]=0x8000;
    uint8_t* collision=out+6+cube;uint8_t* links=collision+6;
    uint16_t duplicates=0;uint32_t head=0,tail=0;
    for(uint16_t step=0;step<256;++step) {
        uint16_t i=step==0?0:step==1?255:step-1;
        const uint8_t* c=colors+8+i*8;
        if(readword(c)&0x4000)continue;
        uint32_t r=readword(c+2)>>(16-resolution),g=readword(c+4)>>(16-resolution),b=readword(c+6)>>(16-resolution);
        uint32_t cell=(r+1)*stride*stride+(g+1)*stride+b+1;
        if(grid[cell]!=0x8000) { uint16_t first=grid[cell];links[i]=links[first];links[first]=uint8_t(i);++duplicates; }
        else { grid[cell]=i;links[i]=uint8_t(i);queue[tail++]=uint16_t(cell); }
    }
    if(!tail)return false;
    int32_t sign=1;
    while(head<tail) {
        uint32_t cell=queue[head++];
        int32_t d[6]={sign,-sign,sign*int32_t(stride),-sign*int32_t(stride),sign*int32_t(stride*stride),-sign*int32_t(stride*stride)};
        for(uint16_t k=0;k<6;++k) {
            uint32_t next=uint32_t(int32_t(cell)+d[k]);
            if(grid[next]==0x8000) { if(tail>=cube)return false;grid[next]=grid[cell];queue[tail++]=uint16_t(next); }
        }
        sign=-sign;
    }
    if(tail!=cube)return false;
    for(uint16_t i=0;i<4;++i)out[i]=colors[i];
    word(out+4,resolution);word(collision,duplicates);
    uint32_t pos=6;
    for(uint32_t r=1;r<=n;++r)for(uint32_t g=1;g<=n;++g)for(uint32_t b=1;b<=n;++b)
        out[pos++]=uint8_t(grid[r*stride*stride+g*stride+b]);
    return true;
}

}
#endif
