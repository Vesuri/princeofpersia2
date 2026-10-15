#ifndef AMIGAXDEV_RECT_BOUNDS_H
#define AMIGAXDEV_RECT_BOUNDS_H
#ifndef __mc68000__
#include <stdint.h>
#endif
namespace RectBounds {
inline uint16_t word(const uint8_t* p) { return uint16_t(uint16_t(p[0])<<8)|p[1]; }
inline void writeWord(uint8_t* p,uint16_t value) {p[0]=uint8_t(value>>8);p[1]=uint8_t(value);}
// EqualRect compares all four coordinates, including empty/inverted bounds.
inline bool equal(const uint8_t* a,const uint8_t* b,bool& same) {
    if(!a || !b)return false;
    same=true;
    for(unsigned i=0;i<8;++i)if(a[i]!=b[i])same=false;
    return true;
}
// UnionRect compares signed coordinates even for empty/inverted rectangles.
// Read both inputs completely before writing so either may alias the output.
template<uint16_t (*Load)(const uint8_t*),void (*Store)(uint8_t*,uint16_t)>
inline bool unite(uint8_t* out,const uint8_t* a,const uint8_t* b) {
    if(!out || !a || !b)return false;
    uint16_t result[4];
    for(unsigned i=0;i<4;++i) {
        int16_t x=int16_t(Load(a+2*i)),y=int16_t(Load(b+2*i));
        result[i]=uint16_t(i<2 ? (x<y?x:y) : (x>y?x:y));
    }
    for(unsigned i=0;i<4;++i) {
        Store(out+2*i,result[i]);
    }
    return true;
}
// SectRect writes the zero rectangle for empty/touching/inverted intersections.
template<uint16_t (*Load)(const uint8_t*),void (*Store)(uint8_t*,uint16_t)>
inline bool intersect(uint8_t* out,const uint8_t* a,const uint8_t* b,bool& nonempty) {
    if(!out || !a || !b)return false;
    int16_t result[4];
    for(unsigned i=0;i<4;++i) {
        int16_t x=int16_t(Load(a+2*i)),y=int16_t(Load(b+2*i));
        result[i]=i<2 ? (x>y?x:y) : (x<y?x:y);
    }
    nonempty=result[0]<result[2] && result[1]<result[3];
    for(unsigned i=0;i<4;++i) {
        uint16_t value=nonempty ? uint16_t(result[i]) : 0;
        Store(out+2*i,value);
    }
    return true;
}

// Explicit accessors let 68020 callers use the shared BigEndianMemory word
// operations, including odd addresses. Defaults retain portable byte accesses.
inline bool unite(uint8_t* out,const uint8_t* a,const uint8_t* b) {
    return unite<word,writeWord>(out,a,b);
}
inline bool intersect(uint8_t* out,const uint8_t* a,const uint8_t* b,bool& nonempty) {
    return intersect<word,writeWord>(out,a,b,nonempty);
}

// OffsetRect / InsetRect, shared from the native Macintosh runtime.
// Components deliberately wrap at 16 bits, including inverted rectangles.
template<uint16_t (*Read)(const uint8_t*),void (*Write)(uint8_t*,uint16_t)>
inline void adjust(uint8_t* rect,uint16_t dh,uint16_t dv,uint16_t operation) {
    Write(rect,Read(rect)+dv);Write(rect+2,Read(rect+2)+dh);
    Write(rect+4,Read(rect+4)+(operation==0xa8a9 ? -dv : dv));
    Write(rect+6,Read(rect+6)+(operation==0xa8a9 ? -dh : dh));
}
inline void adjust(uint8_t* rect,uint16_t dh,uint16_t dv,uint16_t operation) {
    adjust<word,writeWord>(rect,dh,dv,operation);
}

}
#endif
