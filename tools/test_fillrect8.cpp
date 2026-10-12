#include "../src/mac/FillRect8.h"
#include <cstdio>
#include <vector>
int main() {
    // Exercise alignment prefixes and both longword-loop tails, with guards
    // on either side. Includes zero length and spans shorter than alignment.
    for(unsigned offset=0;offset<4;++offset)for(unsigned count=0;count<=65;++count) {
        uint8_t span[80];
        for(unsigned i=0;i<sizeof(span);++i)span[i]=0x53;
        FillRect8::fillSpan(span+4+offset,count,0xa7);
        for(unsigned i=0;i<sizeof(span);++i)
            if(span[i]!=(i>=4+offset && i<4+offset+count?0xa7:0x53))return 4;
    }
    // Whole rectangles retain padding and guards for aligned and odd strides.
    for(unsigned stride=68;stride<=71;++stride)for(unsigned offset=0;offset<4;++offset)
        for(unsigned width=0;width<=65;++width)for(unsigned height=0;height<=4;++height) {
            uint8_t rows[320];for(auto& v:rows)v=0x53;
            FillRect8::fillRows(rows+4+offset,stride,width,height,0xa7);
            for(unsigned i=0;i<sizeof(rows);++i) {
                bool filled=false;
                for(unsigned y=0;y<height;++y)
                    if(i>=4+offset+y*stride && i<4+offset+y*stride+width)filled=true;
                if(rows[i]!=(filled?0xa7:0x53))return 5;
            }
        }
    // Binary fixture: map bounds, port bounds, vis bounds, clip bounds, rect,
    // then the complete 640x480 screen. Output is the complete resulting screen.
    uint8_t bounds[40],drawn[8];
    std::vector<uint8_t> pixels(640*480);
    if(fread(bounds,1,40,stdin)!=40 || fread(pixels.data(),1,pixels.size(),stdin)!=pixels.size())return 1;
    if(!FillRect8::solid(pixels.data(),pixels.size(),640,bounds,bounds+8,bounds+16,bounds+24,bounds+32,255,drawn))return 2;
    return fwrite(pixels.data(),1,pixels.size(),stdout)==pixels.size()?0:3;
}
