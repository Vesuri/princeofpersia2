#ifndef AMIGAXDEV_BITMAP_REGION_H
#define AMIGAXDEV_BITMAP_REGION_H
#include "RegionOps.h"
namespace BitmapRegion {
// Encode one-bit, MSB-first Macintosh bitmap rows with the shared parity-region
// writer. Callers own scratch storage and publish it only after success.
inline bool encode(const uint8_t* pixels,uint32_t bytes,uint16_t stride,
                   const uint8_t* bounds,uint8_t* output,uint16_t capacity,uint16_t& size) {
    size=0;
    if(!pixels || !bounds || !output || capacity<10 || !stride || stride&0xc000)return false;
    int32_t top=RegionRows::get(bounds),left=RegionRows::get(bounds+2);
    int32_t bottom=RegionRows::get(bounds+4),right=RegionRows::get(bounds+6);
    int32_t width=right-left,height=bottom-top;
    if(width<0 || height<0 || uint32_t(width)>uint32_t(stride)*8 ||
       uint32_t(height)>bytes/stride)return false;
    RegionOps::Writer writer(output,capacity);
    for(int32_t row=0;row<height;++row){
        RegionRows::Edges edges{};bool previous=false;
        for(int32_t x=0;x<=width;++x){
            bool set=x<width && (pixels[uint32_t(row)*stride+uint32_t(x)/8]&(0x80>>(x&7)));
            if(set!=previous && !RegionRows::toggle(edges,int16_t(left+x)))return false;
            previous=set;
        }
        if(!writer.row(int16_t(top+row),edges))return false;
    }
    RegionRows::Edges empty{};
    return writer.row(int16_t(bottom),empty) && writer.finish(size);
}
}
#endif
