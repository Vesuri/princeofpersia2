#include "../src/mac/Line8.h"
#include <cstdio>
#include <vector>
#include <cassert>
#include <algorithm>
int main(int argc,char**) {
    unsigned stride=argc==2?640:652,height=argc==2?480:401;uint8_t color=argc==2?16:26;
    uint8_t b[40],drawn[8];std::vector<uint8_t> pixels(stride*height);
    if(fread(b,1,sizeof b,stdin)!=sizeof b || fread(pixels.data(),1,pixels.size(),stdin)!=pixels.size())return 1;
    if(!Line8::solid(pixels.data(),pixels.size(),stride,b,b+8,b+16,b+24,
        int16_t(RectBounds::word(b+32)),int16_t(RectBounds::word(b+34)),
        int16_t(RectBounds::word(b+36)),int16_t(RectBounds::word(b+38)),color,drawn))return 2;
    // Independently scan a contrasting buffer to verify the reported footprint,
    // including empty clips and nonzero PixMap origins in the window fixture.
    std::vector<uint8_t> mask(pixels.size(),color^255);
    assert(Line8::solid(mask.data(),mask.size(),stride,b,b+8,b+16,b+24,
        int16_t(RectBounds::word(b+32)),int16_t(RectBounds::word(b+34)),
        int16_t(RectBounds::word(b+36)),int16_t(RectBounds::word(b+38)),color));
    int mt=int16_t(RectBounds::word(b)),ml=int16_t(RectBounds::word(b+2));
    int top=32767,left=32767,bottom=-32768,right=-32768;
    for(unsigned y=0;y<height;++y)for(unsigned x=0;x<stride;++x)if(mask[y*stride+x]==color) {
        top=std::min(top,int(y)+mt);left=std::min(left,int(x)+ml);
        bottom=std::max(bottom,int(y)+mt+1);right=std::max(right,int(x)+ml+1);
    }
    if(top>=bottom)top=left=bottom=right=0;
    int expected[]={top,left,bottom,right};
    for(unsigned i=0;i<4;++i)assert(int16_t(RectBounds::word(drawn+2*i))==expected[i]);
    return fwrite(pixels.data(),1,pixels.size(),stdout)==pixels.size()?0:3;
}
