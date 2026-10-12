#ifndef POP2_PALETTE8_H
#define POP2_PALETTE8_H
#ifndef POP2_PLATFORM_AMIGA
#include <stdint.h>
#endif
// Measured eight-bit device-table subset: protected white/black endpoints and
// pmTolerant|pmExplicit, zero tolerance. No screen or resource IDs are used.
namespace Palette8 {
inline uint16_t word(const uint8_t* p) { return uint16_t(p[0])<<8|p[1]; }
inline void word(uint8_t* p,uint16_t v) { p[0]=uint8_t(v>>8);p[1]=uint8_t(v); }
inline void longword(uint8_t* p,uint32_t v) { word(p,uint16_t(v>>16));word(p+2,uint16_t(v)); }
inline bool rgb(const uint8_t* p,uint16_t value) {
    return word(p)==value && word(p+2)==value && word(p+4)==value;
}
inline void systemTable(uint8_t* table,uint32_t seed) {
    longword(table,seed);word(table+4,0x8000);word(table+6,255);
    // The 6x6x6 cube excludes black; ten intermediate shades per ramp fill
    // its missing levels, followed by protected black at the final slot.
    static const uint8_t shades[10]={14,13,11,10,8,7,5,4,2,1};
    for(uint16_t i=0;i<256;++i) {
        uint16_t r=0,g=0,b=0;
        if(i<215) {
            r=(5-i/36)*0x3333;g=(5-(i/6)%6)*0x3333;b=(5-i%6)*0x3333;
        } else if(i<255) {
            uint16_t ramp=(i-215)/10,v=uint16_t(shades[(i-215)%10])*0x1111;
            if(ramp==0 || ramp==3)r=v;
            if(ramp==1 || ramp==3)g=v;
            if(ramp==2 || ramp==3)b=v;
        }
        uint8_t* entry=table+8+i*8;
        word(entry,0x0800);word(entry+2,r);word(entry+4,g);word(entry+6,b);
    }
}
inline bool supported(const uint8_t* palette,uint32_t size,uint16_t entryState=0) {
    if(!palette || size!=4112 || word(palette)!=256 || word(palette+2))return false;
    if(!rgb(palette+16,0xffff) || !rgb(palette+16+255*16,0))return false;
    for(uint16_t i=0;i<256;++i) {
        const uint8_t* e=palette+16+i*16;
        if(word(e+6)!=10 || word(e+8)!=0 || word(e+10)!=entryState
           || word(e+12)!=0 || word(e+14)!=0)return false;
    }
    return true;
}
inline bool realize(uint8_t* palette,uint32_t paletteBytes,uint8_t* table,
                    uint32_t tableBytes,uint8_t* privateData,uint32_t privateBytes,
                    uint32_t seed,uint16_t entryState=0,bool reactivating=false) {
    if((reactivating && entryState!=0x800a) || (entryState!=0 && entryState!=0x800a) || !supported(palette,paletteBytes,entryState) || !table || tableBytes!=2056
       || word(table+4)!=0x8000 || word(table+6)!=255 || !privateData || privateBytes!=4
       || !rgb(table+10,0xffff) || !rgb(table+10+255*8,0))return false;
    for(uint16_t i=0;i<256;++i) {
        uint8_t* e=palette+16+i*16;
        // Endpoint colours use the already protected slots. Their duplicate
        // explicit slots retain the previous device colours rather than being
        // overwritten with black or white.
        if(!rgb(e,0) && !rgb(e,0xffff)) {
            uint8_t* target=table+8+i*8;
            // Measured inactive-palette reactivation releases an old explicit
            // slot when its RGB already matches; changed RGB claims the slot.
            bool matching=word(target+2)==word(e) && word(target+4)==word(e+2)
                && word(target+6)==word(e+4);
            word(target,reactivating && matching ? 0 : 0x2000);
            word(target+2,word(e));word(target+4,word(e+2));word(target+6,word(e+4));
        }
        word(e+10,0x800a);
    }
    longword(table,seed);longword(privateData,seed);
    return true;
}
}
#endif
