#ifndef POP2_BITMAP_FONT_H
#define POP2_BITMAP_FONT_H
#ifndef POP2_PLATFORM_AMIGA
#include <stdint.h>
#endif
// Restricted classic FOND/NFNT reader for the embedded compatibility resources.
// Unsupported tables, styles and depths are rejected, never guessed.
class BitmapFont {
public:
    struct Family { uint16_t first,last,size;int16_t bitmap;bool fixedWidth; };
    static uint16_t word(const uint8_t* p) { return uint16_t(p[0])<<8|p[1]; }
    static bool family(const uint8_t* p,uint32_t bytes,int16_t id,Family& out,
                       uint16_t requestedSize=0,uint16_t requestedStyle=0) {
        if(!p || bytes<60 || (word(p)!=0xc000 && word(p)!=0x4000)
           || int16_t(word(p+2))!=id || word(p+50)!=2)return false;
        for(uint16_t n=16;n<50;++n)if(p[n])return false;
        uint16_t first=word(p+4),last=word(p+6),count=word(p+52)+1;
        if(first>last || last>255 || !count || count>64 || bytes!=54+uint32_t(count)*6
           || !word(p+8) || !word(p+14))return false;
        bool found=false;uint32_t previous=0;
        for(uint16_t n=0;n<count;++n) {
            const uint8_t* a=p+54+n*6;
            uint16_t size=word(a),style=word(a+2),bitmap=word(a+4);
            uint32_t key=uint32_t(size)*65536+style;
            if(!size || size>255 || (style!=0 && style!=1 && style!=2 && style!=32 && style!=33)
               || !bitmap || bitmap>32767 || (n && key<=previous))return false;
            previous=key;
            if((!requestedSize && n==0 && !requestedStyle) || (size==requestedSize && style==requestedStyle)) {
                out.first=first;out.last=last;out.size=size;out.bitmap=bitmap;out.fixedWidth=(word(p)&0x8000)!=0;found=true;
            }
        }
        return found;
    }
    bool open(const uint8_t* p,uint32_t bytes,const Family& family) {
        data_=0;
        if(!p || bytes<26 || word(p)!=(family.fixedWidth ? 0x3000 : 0x1000) || word(p+2)!=family.first || word(p+4)!=family.last || word(p+8)!=0)return false;
        uint16_t count=family.last-family.first+2; // includes missing glyph
        uint16_t height=word(p+14),rowWords=word(p+24),ascent=word(p+18),descent=word(p+20),width=word(p+6);
        if(word(p+22)>255 || !height || height>255 || !rowWords || rowWords>4096 || !width || width>255 || word(p+12)>width || ascent+descent!=height || int16_t(word(p+10))!=-int32_t(descent))return false;
        uint32_t locations=26+uint32_t(rowWords)*2*height;
        uint32_t widths=locations+uint32_t(count+1)*2;
        if(widths+uint32_t(count+1)*2!=bytes || 16+uint32_t(word(p+16))*2!=widths || word(p+locations)!=0 || word(p+widths+count*2)!=0xffff)return false;
        for(uint16_t n=0;n<count;++n) {
            uint16_t left=word(p+locations+n*2),right=word(p+locations+(n+1)*2);
            if(left>right || right>uint32_t(rowWords)*16 || right-left>word(p+12) || (word(p+widths+n*2)>>8)!=0 || !(word(p+widths+n*2)&255) || (word(p+widths+n*2)&255)>width || (word(p)==0x3000 && word(p+widths+n*2)!=width))return false;
        }
        data_=p;first_=family.first;last_=family.last;locations_=locations;widths_=widths;rowBytes_=rowWords*2;height_=height;advance_=width;return true;
    }
    uint16_t advance() const { return advance_; }
    uint16_t ascent() const { return data_ ? word(data_+18) : 0; }
    uint16_t descent() const { return data_ ? word(data_+20) : 0; }
    uint16_t leading() const { return data_ ? word(data_+22) : 0; }
    uint16_t charWidth(uint16_t character) const {
        if(!data_)return 0;
        uint16_t index=character>=first_ && character<=last_ ? character-first_ : last_-first_+1;
        return word(data_+widths_+index*2)&255;
    }
    uint16_t height() const { return height_; }
    bool pixel(uint16_t character,uint16_t x,uint16_t y) const {
        if(!data_ || y>=height_)return false;
        uint16_t index=character>=first_ && character<=last_ ? character-first_ : last_-first_+1;
        uint16_t left=word(data_+locations_+index*2),right=word(data_+locations_+(index+1)*2);
        if(x>=right-left)return false;
        uint16_t bit=left+x;
        return (data_[26+uint32_t(y)*rowBytes_+bit/8]&(128>>(bit&7)))!=0;
    }
    static bool nameEquals(const uint8_t* pascal,const uint8_t* name,uint16_t length) {
        if(!pascal || !name || !length || pascal[0]!=length)return false;
        for(uint16_t n=0;n<length;++n) {
            uint8_t a=pascal[n+1],b=name[n];
            if(a>='a' && a<='z')a-='a'-'A';
            if(b>='a' && b<='z')b-='a'-'A';
            if(a!=b)return false;
        }
        return true;
    }
private:
    const uint8_t* data_=0;
    uint32_t locations_=0,widths_=0;
    uint16_t first_=0,last_=0,rowBytes_=0,height_=0,advance_=0;
};
#endif
