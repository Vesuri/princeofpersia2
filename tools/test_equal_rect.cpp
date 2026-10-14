#include <cassert>
#include <cstdint>
#include <cstring>
#include "mac/RectBounds.h"
int main(){
    const int16_t rectangles[][4]={{0,0,10,20},{0,0,10,20},{0,0,0,0},
        {10,20,0,0},{-32768,-1,32767,0},{0,1,10,20}};
    for(const auto& a:rectangles)for(const auto& b:rectangles){
        uint8_t aa[8],bb[8];bool expected=true;
        for(unsigned i=0;i<4;++i){
            RectBounds::writeWord(aa+2*i,uint16_t(a[i]));RectBounds::writeWord(bb+2*i,uint16_t(b[i]));
            expected=expected && a[i]==b[i];
        }
        uint8_t savedA[8],savedB[8];std::memcpy(savedA,aa,8);std::memcpy(savedB,bb,8);
        bool same=!expected;assert(RectBounds::equal(aa,bb,same) && same==expected);
        assert(!std::memcmp(aa,savedA,8) && !std::memcmp(bb,savedB,8));
        assert(RectBounds::equal(aa,aa,same) && same);
        assert(!RectBounds::equal(nullptr,bb,same));
    }
}
