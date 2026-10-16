#include "platform/amiga/Planar8.h"
#include "mac/DirtyBands.h"
#include <cassert>
#include <vector>
#include <cstring>
#include <cstdio>
int main() {
    using Planar8::Rect;
    Rect view={48,64,432,576},previous[64];uint16_t previousCount=0;
    std::vector<uint8_t> chunky(640*480),front(Planar8::bytes),back(Planar8::bytes);
    unsigned seed=7;auto random=[&](){seed=seed*1664525u+1013904223u;return seed;};
    for(unsigned frame=0;frame<400;++frame) {
        Rect dirty[64],work[64];uint16_t n=frame%65,count=0;
        for(unsigned i=0;i<n;++i) {
            int left=random()%512,top=random()%384;
            int right=left+1+random()%(512-left),bottom=top+1+random()%(384-top);
            dirty[i]={(int16_t)(top+48),(int16_t)(left+64),(int16_t)(bottom+48),(int16_t)(right+64)};
            for(int y=top;y<bottom;++y)for(int x=left;x<right;++x)chunky[(y+48)*640+x+64]=frame+x+y;
        }
        assert((DirtyBands::plan<512,384,64>(view,dirty,n,work,64,count)));
        for(unsigned i=0;i<previousCount;++i)if(Planar8::syncNeeded(previous[i],work,count)) {
            auto r=previous[i];
            for(int y=r.top;y<r.bottom;++y)for(unsigned p=0;p<8;++p)
                memcpy(&back[y*512+p*64+r.left/8],&front[y*512+p*64+r.left/8],(r.right-r.left)/8);
        }
        for(unsigned i=0;i<count;++i) {
            auto r=work[i];Rect global={(int16_t)(r.top+48),(int16_t)(r.left+64),(int16_t)(r.bottom+48),(int16_t)(r.right+64)},converted;
            assert(Planar8::convert(chunky.data(),back.data(),view,global,converted));
        }
        Planar8::Mismatch mismatch{};assert(Planar8::verify(chunky.data(),back.data(),view,mismatch));
        previousCount=count;memcpy(previous,work,count*sizeof(Rect));front.swap(back);
    }
    puts("PASS dirty bands: 400 alternating-buffer frames, up to 64 inputs and complete plane preservation");
}
