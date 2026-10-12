#ifndef POP2_POLYGON_RECORD_H
#define POP2_POLYGON_RECORD_H
#include "RectBounds.h"
namespace PolygonRecord {
inline void put(uint8_t* p,uint16_t value) { p[0]=uint8_t(value>>8);p[1]=uint8_t(value); }
// Measured QuickDraw recording: bounds stay empty until ClosePoly. The first
// LineTo stores both endpoints; following connected lines append one point.
inline uint16_t growth(const uint8_t* p,uint32_t capacity,int16_t x,int16_t y) {
    if(!p || capacity<10)return 0;
    uint16_t n=RectBounds::word(p);
    if(n>capacity || n<10 || (n-10)%4 || (n!=10 && n<18))return 0;
    for(unsigned i=2;i<10;++i)if(p[i])return 0;
    if(n>10 && (RectBounds::word(p+n-4)!=uint16_t(y)
                || RectBounds::word(p+n-2)!=uint16_t(x)))return 0;
    uint32_t next=uint32_t(n)+(n==10 ? 8 : 4);
    return next<=32766 ? uint16_t(next) : 0;
}
inline bool append(uint8_t* p,uint32_t capacity,int16_t x0,int16_t y0,int16_t x1,int16_t y1) {
    uint16_t next=growth(p,capacity,x0,y0);
    if(!next || next>capacity)return false;
    uint16_t n=RectBounds::word(p);
    if(n==10) { put(p+n,uint16_t(y0));put(p+n+2,uint16_t(x0));n+=4; }
    put(p+n,uint16_t(y1));put(p+n+2,uint16_t(x1));
    put(p,next);return true;
}
inline bool close(uint8_t* p,uint32_t capacity) {
    if(!p || capacity<26)return false;
    uint16_t n=RectBounds::word(p);
    if(n<26 || n>capacity || (n-10)%4)return false;
    for(unsigned i=2;i<10;++i)if(p[i])return false;
    // Only the reached explicitly closed chain is accepted so far.
    if(RectBounds::word(p+10)!=RectBounds::word(p+n-4)
       || RectBounds::word(p+12)!=RectBounds::word(p+n-2))return false;
    int16_t top=int16_t(RectBounds::word(p+10)),bottom=top;
    int16_t left=int16_t(RectBounds::word(p+12)),right=left;
    for(uint16_t i=14;i<n;i+=4) {
        int16_t y=int16_t(RectBounds::word(p+i)),x=int16_t(RectBounds::word(p+i+2));
        if(y<top)top=y;if(y>bottom)bottom=y;if(x<left)left=x;if(x>right)right=x;
    }
    put(p+2,uint16_t(top));put(p+4,uint16_t(left));
    put(p+6,uint16_t(bottom));put(p+8,uint16_t(right));return true;
}
}
#endif
