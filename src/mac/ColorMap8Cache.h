#ifndef POP2_COLOR_MAP8_CACHE_H
#define POP2_COLOR_MAP8_CACHE_H
#include "GWorld8.h"
// Color QuickDraw identifies colour environments by ctSeed. Own the small
// translation, not either table; heap relocation changes the pointer key.
class ColorMap8Cache {
    struct Entry {
        const uint8_t *source,*destination,*inverse;
        uint32_t sourceSeed,destinationSeed;
        uint16_t sourceFlags,destinationFlags,resolution;
        uint8_t colors[256];
        bool valid;
    };
    Entry entries_[4]{};
    uint16_t next_=0;
    static uint32_t seed(const uint8_t* p) {
        return uint32_t(p[0])<<24|uint32_t(p[1])<<16|uint32_t(p[2])<<8|p[3];
    }
public:
    bool map(const uint8_t* source,const uint8_t* destination,const uint8_t* inverse,
             const uint8_t*& colors,bool& hit) {
        hit=false;colors=nullptr;
        if(!source || !destination || !inverse
           || GWorld8::readword(source+6)!=255 || GWorld8::readword(destination+6)!=255)return false;
        const uint32_t ss=seed(source),ds=seed(destination);
        const uint16_t sf=GWorld8::readword(source+4),df=GWorld8::readword(destination+4);
        const uint16_t resolution=GWorld8::readword(inverse+4);
        if((sf!=0 && sf!=0x8000) || (df!=0 && df!=0x8000)
           || seed(inverse)!=ds || (resolution!=4 && resolution!=5))return false;
        for(uint16_t i=0;i<4;++i) {
            const Entry& e=entries_[i];
            if(e.valid && e.source==source && e.destination==destination && e.inverse==inverse
               && e.sourceSeed==ss && e.destinationSeed==ds
               && e.sourceFlags==sf && e.destinationFlags==df && e.resolution==resolution) {
                colors=e.colors;hit=true;return true;
            }
        }
        Entry& e=entries_[next_];next_=(next_+1)&3;e.valid=false;
        for(uint16_t i=0;i<256;++i) {
            const uint8_t* rgb=source+8+uint32_t(i)*8;uint16_t index;
            if((sf==0 && GWorld8::readword(rgb)!=i)
               || !GWorld8::colorIndex(destination,inverse,rgb+2,index))return false;
            e.colors[i]=uint8_t(index);
        }
        e.source=source;e.destination=destination;e.inverse=inverse;
        e.sourceSeed=ss;e.destinationSeed=ds;e.sourceFlags=sf;e.destinationFlags=df;
        e.resolution=resolution;e.valid=true;colors=e.colors;return true;
    }
};
#endif
