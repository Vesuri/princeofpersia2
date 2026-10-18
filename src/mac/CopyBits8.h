#ifndef AMIGAXDEV_COPY_BITS8_H
#define AMIGAXDEV_COPY_BITS8_H
#include "RectBounds.h"
#include "BigEndianMemory.h"
#include "RegionRows.h"
namespace CopyBits8 {
// Optional change report for distinct, unscaled, unmasked identity copies.
// Row offsets are relative to left; an empty row has left == right. Other
// supported copy forms still copy normally and leave valid false.
struct RowSpan { uint16_t left,right; };
// The optional copier must preserve bytes outside each row and report bounds
// containing every changed byte. Inputs are distinct owned pixel buffers.
using ChangedRows = void (*)(const uint8_t*,uint8_t*,uint32_t,uint32_t,uint32_t,uint32_t,RowSpan*);
struct Changes {
    RowSpan* rows;uint32_t capacity,count;int32_t top,left;bool valid;ChangedRows copy;
};
inline int32_t coord(const uint8_t* r,unsigned i) { return int16_t(BigEndianMemory::read16(r+2*i)); }
// Owned, distinct pixel buffers need no overlap handling. Permit arbitrary
// byte alignment without violating aliasing rules; the target is 68020+.
inline void copySpan(const uint8_t* in,uint8_t* out,uint32_t count) {
    while(count>=16) {
#ifdef __mc68000__
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
#ifdef __mc68000__
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
// The row loop has only its live pointers and strides, independent of the
// clipping/scaling setup. In particular, do not spill the pointers per row.
__attribute__((noinline)) inline void directRows(const uint8_t* in,uint8_t* out,
        uint32_t width,uint32_t height,uint16_t srcStride,uint16_t dstStride) {
    if(!height)return;
    do {
        copySpan(in,out,width);
        in+=srcStride;out+=dstStride;
    } while(--height);
}
// Keep the thumbnail scaler out of the common unscaled copy's stack frame.
__attribute__((noinline)) inline bool scale(const uint8_t* src,uint16_t srcStride,const uint8_t* srcMap,
                 uint8_t* dst,uint16_t dstStride,const uint8_t* dstMap,
                 const uint8_t* from,const uint8_t* to,const uint8_t* port,
                 const uint8_t* vis,const uint8_t* clip,uint8_t* drawn,const uint8_t* colors,
                 const uint8_t* mask,uint16_t maskBytes,RegionRows::ValidationCache* maskCache,
                 int32_t h,int32_t w,int32_t targetH,int32_t targetW) {
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
inline void intersect(int32_t* r,const uint8_t* box,int32_t dy=0,int32_t dx=0) {
    const int32_t t=coord(box,0)+dy,l=coord(box,1)+dx;
    const int32_t b=coord(box,2)+dy,v=coord(box,3)+dx;
    if(r[0]<t)r[0]=t;if(r[1]<l)r[1]=l;
    if(r[2]>b)r[2]=b;if(r[3]>v)r[3]=v;
}
inline bool validMap(const uint8_t* bounds,uint16_t stride,uint32_t bytes) {
    const int32_t h=coord(bounds,2)-coord(bounds,0),w=coord(bounds,3)-coord(bounds,1);
    return h>0 && w>0 && uint32_t(w)<=stride && uint32_t(h)*stride<=bytes;
}
// Vette's unscaled clipping equations, applied to byte pixels. The caller
// supplies a measured colour map when the colour environments differ, and
// establishes owned buffers. Same-buffer copies require an unscaled, unmasked
// transfer with identical strides; other aliasing forms remain unsupported.
inline bool copy(const uint8_t* src,uint32_t srcBytes,uint16_t srcStride,const uint8_t* srcMap,
                 uint8_t* dst,uint32_t dstBytes,uint16_t dstStride,const uint8_t* dstMap,
                 const uint8_t* from,const uint8_t* to,const uint8_t* port,
                 const uint8_t* vis,const uint8_t* clip,uint8_t* drawn,const uint8_t* colors=0,
                 const uint8_t* mask=0,uint16_t maskBytes=0,RegionRows::ValidationCache* maskCache=nullptr,Changes* changes=nullptr) {
    if(changes)changes->valid=false;
    if(!src || !dst || !srcMap || !dstMap || !from || !to
       || !port || !vis || !clip || !drawn)return false;
    if(!validMap(srcMap,srcStride,srcBytes) || !validMap(dstMap,dstStride,dstBytes))return false;
    const int32_t h=coord(from,2)-coord(from,0),w=coord(from,3)-coord(from,1);
    const int32_t targetH=from==to ? h : coord(to,2)-coord(to,0);
    const int32_t targetW=from==to ? w : coord(to,3)-coord(to,1);
    if(h<=0 || w<=0 || targetH<=0 || targetW<=0)return false;
    if(src==dst && (mask || colors || srcStride!=dstStride || h!=targetH || w!=targetW))return false;
    if(h!=targetH || w!=targetW) {
        return scale(src,srcStride,srcMap,dst,dstStride,dstMap,from,to,port,vis,clip,drawn,
                     colors,mask,maskBytes,maskCache,h,w,targetH,targetW);
    }
    RegionRows::Cursor rows;
    if(mask && !rows.begin(mask,maskBytes,maskCache))return false;
    int32_t limits[4]={coord(to,0),coord(to,1),coord(to,2),coord(to,3)};
    // Most foreground masks miss the actor's destination box. The complete
    // stream is validated even here; a damaged offscreen tail must not succeed.
    if(mask) {
        intersect(limits,mask+2);
        if(limits[0]>=limits[2] || limits[1]>=limits[3]) {
            for(unsigned i=0;i<8;++i)drawn[i]=0;
            return true;
        }
    }
    const int32_t dy=from==to ? 0 : coord(to,0)-coord(from,0);
    const int32_t dx=from==to ? 0 : coord(to,1)-coord(from,1);
    intersect(limits,srcMap,dy,dx);
    intersect(limits,dstMap);intersect(limits,port);
    intersect(limits,vis);intersect(limits,clip);
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
    if(src==dst) {
        // Vette's overlap traversal, applied to byte pixels. Copy the last row
        // first when its destination follows its source, and reverse each span
        // when necessary. Clipping is resolved before selecting the direction.
        const uint8_t* in=source+(sourceX+limits[1]);
        uint8_t* out=target+(destinationX+limits[1]);
        const uint32_t count=uint32_t(limits[3]-limits[1]);
        const int32_t height=limits[2]-limits[0];
        const bool backward=out>in;
        for(int32_t i=0;i<height;++i) {
            const int32_t row=backward?height-1-i:i;
            const uint8_t* a=in+row*srcStride;uint8_t* b=out+row*dstStride;
            if(backward)for(uint32_t x=count;x>0;--x)b[x-1]=a[x-1];
            else for(uint32_t x=0;x<count;++x)b[x]=a[x];
        }
        return true;
    }
    if(!mask) {
        const uint8_t* in=source+(sourceX+limits[1]);
        uint8_t* out=target+(destinationX+limits[1]);
        const uint32_t count=uint32_t(limits[3]-limits[1]);
        if(!colors) {
            const uint32_t height=uint32_t(limits[2]-limits[0]);
            if(changes && changes->rows && changes->copy && height<=changes->capacity) {
                changes->copy(in,out,count,height,srcStride,dstStride,changes->rows);
                changes->top=limits[0];changes->left=limits[1];changes->count=height;changes->valid=true;
            } else directRows(in,out,count,height,srcStride,dstStride);
        } else for(int32_t y=limits[0];y<limits[2];++y,in+=srcStride,out+=dstStride)
            for(uint32_t x=0;x<count;++x)out[x]=colors[in[x]];
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
