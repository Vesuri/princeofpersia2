#ifndef AMIGAXDEV_BITMAP_TEXT8_H
#define AMIGAXDEV_BITMAP_TEXT8_H
#include "BitmapFont.h"
#include "RectBounds.h"
// Integer-advance, monochrome srcOr text. Clipping and pixel addressing reuse
// Alone in the Dark's Text8 renderer; glyphs come from the selected NFNT.
namespace BitmapText8 {
inline bool draw(uint8_t* pixels,uint32_t capacity,uint16_t stride,
                 const uint8_t* map,const uint8_t* port,const uint8_t* vis,const uint8_t* clip,
                 const BitmapFont& font,const uint8_t* text,uint16_t count,
                 int16_t baseline,int16_t& pen,uint8_t color,uint8_t* dirty) {
    if(!pixels || !map || !port || !vis || !clip || !dirty || (!text && count))return false;
    int32_t mt=int16_t(RectBounds::word(map)),ml=int16_t(RectBounds::word(map+2));
    int32_t mb=int16_t(RectBounds::word(map+4)),mr=int16_t(RectBounds::word(map+6));
    if(mb<=mt || mr<=ml || uint32_t(mr-ml)>stride || uint32_t(mb-mt)*stride>capacity)return false;
    int32_t finalPen=pen;
    for(unsigned i=0;i<count;++i)finalPen+=font.integerWidth(text[i]);
    if(finalPen>32767)return false;
    bool visible;RectBounds::intersect(dirty,map,port,visible);
    if(visible)RectBounds::intersect(dirty,dirty,vis,visible);
    if(visible)RectBounds::intersect(dirty,dirty,clip,visible);
    int32_t top=int16_t(RectBounds::word(dirty)),left=int16_t(RectBounds::word(dirty+2));
    int32_t bottom=int16_t(RectBounds::word(dirty+4)),right=int16_t(RectBounds::word(dirty+6));
    int32_t position=pen;
    for(unsigned i=0;visible && i<count;++i){
        uint8_t c=text[i];
        for(unsigned y=0;y<font.height();++y)for(unsigned x=0;x<font.imageWidth(c);++x){
            if(!font.ink(c,x,y))continue;
            int32_t px=position+font.leftBearing(c)+int32_t(x),py=int32_t(baseline)-font.ascent()+int32_t(y);
            if(px>=left && px<right && py>=top && py<bottom)pixels[uint32_t(py-mt)*stride+uint32_t(px-ml)]=color;
        }
        position+=font.integerWidth(c);
    }
    pen=int16_t(finalPen);return true;
}
}
#endif
