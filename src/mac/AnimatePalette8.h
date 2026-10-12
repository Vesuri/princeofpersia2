#ifndef POP2_ANIMATE_PALETTE8_H
#define POP2_ANIMATE_PALETTE8_H
#include "Palette8.h"
// MEASURED Prince uses pmAnimated|pmExplicit (12), with tolerant (2)
// protected white/black endpoints. This differs from the donor's usage 10.
namespace AnimatePalette8 {
inline bool valid(const uint8_t* palette,uint32_t size) {
    if(!palette || size!=4112 || Palette8::word(palette)!=256)return false;
    for(unsigned i=0;i<256;++i) {
        const uint8_t* e=palette+16+16*i;
        if(Palette8::word(e+8) || Palette8::word(e+14))return false;
        if(Palette8::word(e+6)!=(i==0 || i==255 ? 2 : 12))return false;
    }
    return Palette8::rgb(palette+16,65535) && Palette8::rgb(palette+16+255*16,0);
}
inline bool activate(uint8_t* palette,uint32_t size,uint8_t* table,uint32_t tableBytes) {
    if(!valid(palette,size) || !table || tableBytes!=2056 || Palette8::word(table+4)!=0x8000 || Palette8::word(table+6)!=255)return false;
    for(unsigned i=0;i<256;++i) {
        uint8_t* e=palette+16+16*i;
        Palette8::word(e+10,0x8000|Palette8::word(e+6));
        if(i!=0 && i!=255) {
            Palette8::word(e+12,0x8000|i);
            uint8_t* c=table+8+i*8;Palette8::word(c,0x4000);
            for(unsigned j=0;j<3;++j)Palette8::word(c+2+2*j,Palette8::word(e+2*j));
        }
    }
    return true;
}
inline bool animate(uint8_t* palette,uint32_t size,uint8_t* table,uint32_t tableBytes,
                    const uint8_t* source,uint32_t sourceBytes,uint16_t first,
                    uint16_t destination,uint16_t count,bool active) {
    if(!valid(palette,size) || !source || sourceBytes<8 || Palette8::word(source+4)
       || Palette8::word(source+6)!=255 || first>256 || count>256-first
       || destination<1 || destination>255 || count>255-destination
       || sourceBytes<8+8UL*(first+count) || !table || tableBytes!=2056)return false;
    for(unsigned i=0;i<count;++i) {
        const uint8_t* c=source+8+8*(first+i);uint8_t* e=palette+16+16*(destination+i);
        if(Palette8::word(c)!=first+i || (active && (Palette8::word(e+10)!=0x800c || Palette8::word(e+12)!=(0x8000|destination+i))))return false;
    }
    for(unsigned i=0;i<count;++i) {
        const uint8_t* c=source+8+8*(first+i);uint8_t* e=palette+16+16*(destination+i);
        for(unsigned j=0;j<3;++j) {
            // MEASURED AnimatePalette replaces the high byte and retains the low
            // byte already in each realised palette entry.
            uint16_t value=(Palette8::word(c+2+j*2)&0xff00)|e[j*2+1];Palette8::word(e+j*2,value);
            if(active)Palette8::word(table+8+8*(destination+i)+2+j*2,value);
        }
    }
    return true;
}
}
#endif
