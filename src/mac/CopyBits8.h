#ifndef POP2_COPY_BITS8_H
#define POP2_COPY_BITS8_H
#include "RectBounds.h"
#include "RegionRows.h"
namespace CopyBits8 {
inline int32_t coord(const uint8_t* r,unsigned i) { return int16_t(RectBounds::word(r+2*i)); }
// Owned, distinct pixel buffers need no overlap handling. Permit arbitrary
// byte alignment without violating aliasing rules; the target is 68020+.
inline void copySpan(const uint8_t* in,uint8_t* out,uint32_t count) {
    while(count>=16) {
#ifdef POP2_PLATFORM_AMIGA
        __asm__ volatile("move.l (%0)+,(%1)+\n\tmove.l (%0)+,(%1)+\n\t"
                         "move.l (%0)+,(%1)+\n\tmove.l (%0)+,(%1)+"
                         : "+a"(in), "+a"(out) : : "cc", "memory");
#else
        typedef uint32_t PixelWord __attribute__((__may_alias__,__aligned__(1)));
        for(unsigned i=0;i<4;++i)
            reinterpret_cast<PixelWord*>(out)[i]=reinterpret_cast<const PixelWord*>(in)[i];
        in+=16;out+=16;
#endif
        count-=16;
    }
    while(count>=4) {
#ifdef POP2_PLATFORM_AMIGA
        // GCC expands an alignment-one integer load/store into byte shifts.
        // The 68020+ supports this longword transfer at every byte alignment.
        __asm__ volatile("move.l (%0)+,(%1)+" : "+a"(in), "+a"(out) : : "cc", "memory");
#else
        typedef uint32_t PixelWord __attribute__((__may_alias__,__aligned__(1)));
        *reinterpret_cast<PixelWord*>(out)=*reinterpret_cast<const PixelWord*>(in);
        in+=4;out+=4;
#endif
        count-=4;
    }
    while(count--)*out++=*in++;
}
// Vette's unscaled clipping equations, applied to byte pixels. The caller
// supplies a measured colour map when the colour environments differ, and
// establishes distinct owned buffers.
inline bool copy(const uint8_t* src,uint32_t srcBytes,uint16_t srcStride,const uint8_t* srcMap,
                 uint8_t* dst,uint32_t dstBytes,uint16_t dstStride,const uint8_t* dstMap,
                 const uint8_t* from,const uint8_t* to,const uint8_t* port,
                 const uint8_t* vis,const uint8_t* clip,uint8_t* drawn,const uint8_t* colors=0,
                 const uint8_t* mask=0,uint16_t maskBytes=0,RegionRows::ValidationCache* maskCache=nullptr) {
    if(!src || !dst || src==dst || !srcMap || !dstMap || !from || !to
       || !port || !vis || !clip || !drawn)return false;
    const uint8_t* maps[2]={srcMap,dstMap};uint32_t sizes[2]={srcBytes,dstBytes};
    uint16_t strides[2]={srcStride,dstStride};
    for(unsigned i=0;i<2;++i) {
        int32_t h=coord(maps[i],2)-coord(maps[i],0),w=coord(maps[i],3)-coord(maps[i],1);
        if(h<=0 || w<=0 || uint32_t(w)>strides[i] || uint32_t(h)*strides[i]>sizes[i])return false;
    }
    int32_t h=coord(from,2)-coord(from,0),w=coord(from,3)-coord(from,1);
    const int32_t targetH=coord(to,2)-coord(to,0),targetW=coord(to,3)-coord(to,1);
    if(h<=0 || w<=0 || targetH<=0 || targetW<=0)return false;
    if(h!=targetH || w!=targetW) {
        // Reached save-slot thumbnail: a contained source, distinct owned buffers,
        // srcCopy/ditherCopy with exact indexed colours. Sample pixel centres.
        for(unsigned i=0;i<4;++i)
            if(i<2 ? coord(from,i)<coord(srcMap,i) : coord(from,i)>coord(srcMap,i))return false;
        int32_t limits[4];
        for(unsigned i=0;i<4;++i) {
            int32_t bound=coord(to,i);
            const uint8_t* boxes[4]={dstMap,port,vis,clip};
            for(unsigned j=0;j<4;++j) {
                const int32_t value=coord(boxes[j],i);
                bound=i<2 ? (bound>value?bound:value) : (bound<value?bound:value);
            }
            limits[i]=bound;
        }
        RegionRows::Cursor rows;
        if(mask && !rows.begin(mask,maskBytes,maskCache))return false;
        if(mask)for(unsigned i=0;i<4;++i) {
            const int32_t value=coord(mask+2,i);
            limits[i]=i<2 ? (limits[i]>value?limits[i]:value) : (limits[i]<value?limits[i]:value);
        }
        const bool visible=limits[0]<limits[2] && limits[1]<limits[3];
        for(unsigned i=0;i<4;++i) {
            const uint16_t value=visible?uint16_t(limits[i]):0;
            drawn[2*i]=uint8_t(value>>8);drawn[2*i+1]=uint8_t(value);
        }
        if(!visible)return true;
        for(int32_t y=limits[0];y<limits[2];++y) {
            const uint32_t sy=uint32_t(coord(from,0)-coord(srcMap,0))
                +(uint32_t(y-coord(to,0))*uint32_t(h)+uint32_t(h)/2)/uint32_t(targetH);
            if(mask && !rows.advance(int16_t(y)))return false;
            const uint16_t spans=mask?rows.edges.count:2;
            for(uint16_t span=0;span<spans;span+=2) {
                int32_t left=limits[1],right=limits[3];
                if(mask) {
                    if(left<rows.edges.x[span])left=rows.edges.x[span];
                    if(right>rows.edges.x[span+1])right=rows.edges.x[span+1];
                }
                for(int32_t x=left;x<right;++x) {
                    const uint32_t sx=uint32_t(coord(from,1)-coord(srcMap,1))
                        +(uint32_t(x-coord(to,1))*uint32_t(w)+uint32_t(w)/2)/uint32_t(targetW);
                    const uint8_t value=src[sy*srcStride+sx];
                    dst[uint32_t(y-coord(dstMap,0))*dstStride+uint32_t(x-coord(dstMap,1))]=colors?colors[value]:value;
                }
            }
        }
        return true;
    }
    int32_t limits[4];
    for(unsigned i=0;i<4;++i) {
        int32_t bound=coord(to,i),source=coord(to,i<2?i:i-2)+coord(srcMap,i)-coord(from,i<2?i:i-2);
        bound=i<2 ? (bound>source?bound:source) : (bound<source?bound:source);
        const uint8_t* boxes[4]={dstMap,port,vis,clip};
        for(unsigned j=0;j<4;++j) {
            int32_t v=coord(boxes[j],i);bound=i<2 ? (bound>v?bound:v) : (bound<v?bound:v);
        }
        limits[i]=bound;
    }
    RegionRows::Cursor rows;
    if(mask && !rows.begin(mask,maskBytes,maskCache))return false;
    // Region bounds are in destination coordinates. Validate the complete
    // stream first, including empty intersections, then avoid traversing rows
    // that cannot contain any masked pixels (common for foreground polygons).
    if(mask)for(unsigned i=0;i<4;++i) {
        int32_t bound=coord(mask+2,i);
        limits[i]=i<2 ? (limits[i]>bound ? limits[i] : bound)
                     : (limits[i]<bound ? limits[i] : bound);
    }
    bool nonempty=limits[0]<limits[2] && limits[1]<limits[3];
    for(unsigned i=0;i<4;++i) {
        uint16_t v=nonempty?uint16_t(limits[i]):0;drawn[2*i]=uint8_t(v>>8);drawn[2*i+1]=uint8_t(v);
    }
    if(!nonempty)return true;
    const int32_t sourceX=coord(from,1)-coord(to,1)-coord(srcMap,1);
    const int32_t destinationX=-coord(dstMap,1);
    // Row addresses advance by stride; rectangle fields are read once.
    const uint8_t* source=src+uint32_t(coord(from,0)+limits[0]-coord(to,0)-coord(srcMap,0))*srcStride;
    uint8_t* target=dst+uint32_t(limits[0]-coord(dstMap,0))*dstStride;
    if(!mask) {
        const uint8_t* in=source+(sourceX+limits[1]);
        uint8_t* out=target+(destinationX+limits[1]);
        const uint32_t count=uint32_t(limits[3]-limits[1]);
        for(int32_t y=limits[0];y<limits[2];++y,in+=srcStride,out+=dstStride) {
            if(colors)for(uint32_t x=0;x<count;++x)out[x]=colors[in[x]];
            else copySpan(in,out,count);
        }
        return true;
    }
    for(int32_t y=limits[0];y<limits[2];++y,source+=srcStride,target+=dstStride) {
        if(!rows.advance(int16_t(y)))return false;
        const uint16_t spans=rows.edges.count;
        for(uint16_t span=0;span<spans;span+=2) {
            int32_t left=limits[1],right=limits[3];
            if(left<rows.edges.x[span])left=rows.edges.x[span];
            if(right>rows.edges.x[span+1])right=rows.edges.x[span+1];
            if(left>=right)continue;
            const uint8_t* in=source+(sourceX+left);
            uint8_t* out=target+(destinationX+left);
            int32_t count=right-left;
            if(colors)while(count--)*out++=colors[*in++];
            else copySpan(in,out,uint32_t(count));
        }
    }
    return true;
}
}
#endif
