#ifndef AMIGAXDEV_FILL_RECT8_H
#define AMIGAXDEV_FILL_RECT8_H
#include "RectBounds.h"
#include "RegionRows.h"
namespace FillRect8 {
// Align longword stores to avoid split bus accesses on narrow page strips.
// Byte stores handle only the alignment prefix and the short tail.
inline void fillSpan(uint8_t* out,uint32_t count,uint8_t color) {
    while(count && ((unsigned long)out&3)) {*out++=color;--count;}
    const uint32_t word=uint32_t(color)*0x01010101UL;
#ifdef __mc68000__
    if(count>=16) {
        uint32_t blocks=count/16;count%=16;
        __asm__ volatile("1: move.l %2,(%0)+\n\tmove.l %2,(%0)+\n\t"
                         "move.l %2,(%0)+\n\tmove.l %2,(%0)+\n\t"
                         "subq.l #1,%1\n\tbne.s 1b"
                         : "+&a"(out),"+&d"(blocks) : "d"(word) : "cc", "memory");
    }
#else
    while(count>=16) {
        for(unsigned i=0;i<16;++i)*out++=color;
        count-=16;
    }
#endif
    while(count>=4) {
#ifdef __mc68000__
        __asm__ volatile("move.l %1,(%0)+" : "+a"(out) : "d"(word) : "cc", "memory");
#else
        for(unsigned i=0;i<4;++i)*out++=color;
#endif
        count-=4;
    }
    while(count--)*out++=color;
#ifndef __mc68000__
    (void)word;
#endif
}
// Keep rectangle loop state separate from clipping/region decoding. For aligned
// strides every row has the same prefix, longword count and tail.
__attribute__((noinline)) inline void fillRows(uint8_t* row,uint16_t stride,
                                               uint16_t width,uint16_t height,uint8_t color) {
    if(!width || !height)return;
#ifdef __mc68000__
    if(!(stride&3)) {
        uint16_t prefix=uint16_t(-(unsigned long)row)&3;
        if(prefix>width)prefix=width;
        const uint32_t word=uint32_t(color)*0x01010101UL;
        const uint16_t longs=(width-prefix)/4,tail=(width-prefix)&3;
        uint8_t* out;uint16_t counter;
        --height;
        __asm__ volatile(
            "1: move.l %[row],%[out]\n\t"
            "move.w %[prefix],%[n]\n\tbra.s 3f\n"
            "2: move.b %[ink],(%[out])+\n"
            "3: dbra %[n],2b\n\t"
            "move.w %[longs],%[n]\n\tbra.s 5f\n"
            "4: move.l %[ink],(%[out])+\n"
            "5: dbra %[n],4b\n\t"
            "move.w %[tail],%[n]\n\tbra.s 7f\n"
            "6: move.b %[ink],(%[out])+\n"
            "7: dbra %[n],6b\n\t"
            "adda.l %[stride],%[row]\n\tdbra %[height],1b"
            : [row] "+&a"(row),[height] "+&d"(height),[out] "=&a"(out),[n] "=&d"(counter)
            : [prefix] "d"(prefix),[longs] "d"(longs),
              [tail] "d"(tail),[ink] "d"(word),[stride] "a"(uint32_t(stride))
            : "cc","memory");
        return;
    }
#endif
    while(height--) {fillSpan(row,width,color);row+=stride;}
}
// All bounds use the selected port's signed local coordinates. The returned
// rectangle is the actual write area, for conversion to display coordinates.
inline bool solid(uint8_t* pixels,uint32_t capacity,uint16_t stride,
                  const uint8_t* map,const uint8_t* port,const uint8_t* vis,
                  const uint8_t* clip,const uint8_t* rect,uint8_t color,uint8_t* drawn,
                  const uint8_t* mask=0,uint16_t maskBytes=0) {
    if(!pixels || !map || !port || !vis || !clip || !rect || !drawn)return false;
    int32_t mt=int16_t(RectBounds::word(map)),ml=int16_t(RectBounds::word(map+2));
    int32_t mb=int16_t(RectBounds::word(map+4)),mr=int16_t(RectBounds::word(map+6));
    if(mb<=mt || mr<=ml || uint32_t(mr-ml)>stride
       || uint32_t(mb-mt)*stride>capacity)return false;
    RegionRows::Cursor rows;if(mask && !rows.begin(mask,maskBytes))return false;
    bool nonempty;
    RectBounds::intersect(drawn,rect,map,nonempty);
    const uint8_t* limits[3]={port,vis,clip};
    for(unsigned i=0;i<3 && nonempty;++i)RectBounds::intersect(drawn,drawn,limits[i],nonempty);
    if(!nonempty)return true;
    int32_t top=int16_t(RectBounds::word(drawn)),left=int16_t(RectBounds::word(drawn+2));
    int32_t bottom=int16_t(RectBounds::word(drawn+4)),right=int16_t(RectBounds::word(drawn+6));
    if(!mask) {
        uint8_t* row=pixels+uint32_t(top-mt)*stride+left-ml;
        const uint32_t width=uint32_t(right-left);
        fillRows(row,stride,uint16_t(width),uint16_t(bottom-top),color);
        return true;
    }
    for(int32_t y=top;y<bottom;++y) {
        uint8_t* row=pixels+uint32_t(y-mt)*stride;
        if(mask && !rows.advance(int16_t(y)))return false;
        const uint16_t count=mask?rows.edges.count:2;
        for(uint16_t i=0;i<count;i+=2) {
            int32_t start=left,end=right;
            if(mask) {if(start<rows.edges.x[i])start=rows.edges.x[i];if(end>rows.edges.x[i+1])end=rows.edges.x[i+1];}
            if(start<end)fillSpan(row+start-ml,uint32_t(end-start),color);
        }
    }
    return true;
}
}
#endif
