#ifndef POP2_PLANAR8_H
#define POP2_PLANAR8_H
#ifndef POP2_PLATFORM_AMIGA
#include <stdint.h>
#endif

// Integer reference converter for the fixed 512x384 view of a 640x480 Mac
// device. Plane zero is the least significant pixel bit. Rows interleave eight
// 64-byte planes. The assembly converter must agree with this representation.
namespace Planar8 {
static const uint16_t width=512, height=384, planes=8, planeRow=64, rowBytes=512;
static const uint32_t bytes=uint32_t(rowBytes)*height;
struct Rect { int16_t top,left,bottom,right; };

inline bool contains(const Rect& outer,const Rect& inner) {
    return outer.top<=inner.top && outer.left<=inner.left
        && outer.bottom>=inner.bottom && outer.right>=inner.right;
}

// Kalms needs a width divisible by 32, but its output may start on any word.
// Anchoring the block at a 16-pixel boundary avoids expanding a narrow actor
// across two fixed 32-pixel columns. Keep the final block inside the viewport.
inline void alignSpan(Rect& r) {
    r.left=int16_t(r.left&~15);
    r.right=int16_t(r.left+((r.right-r.left+31)&~31));
    if(r.right>width) {r.left-=r.right-width;r.right=width;}
}

// Union only when it covers no additional pixels: containment, or an exact
// shared edge/overlap in one axis. Partial overlaps stay separate.
inline bool mergeLosslessly(const Rect& a,const Rect& b) {
    if(contains(a,b) || contains(b,a))return true;
    const int16_t left=a.left<b.left ? a.left : b.left;
    const int16_t right=a.right>b.right ? a.right : b.right;
    return (a.left==b.left && a.right==b.right && a.top<=b.bottom && a.bottom>=b.top)
        || (a.top==b.top && a.bottom==b.bottom && a.left<=b.right && a.right>=b.left
            && !((right-left)&31));
}

// Add an already normalized, non-empty rectangle. Merge only if the resulting
// width still satisfies Kalms; differently anchored overlaps may stay separate.
inline void append(Rect* rects,uint16_t& count,uint16_t capacity,Rect r) {
    bool merged;
    do {
        merged=false;
        for(uint16_t i=0;i<count;++i) {
            if(!mergeLosslessly(r,rects[i]))continue;
            if(rects[i].top<r.top)r.top=rects[i].top;
            if(rects[i].left<r.left)r.left=rects[i].left;
            if(rects[i].bottom>r.bottom)r.bottom=rects[i].bottom;
            if(rects[i].right>r.right)r.right=rects[i].right;
            rects[i]=rects[--count];merged=true;break;
        }
    } while(merged);
    if(count<capacity) {rects[count++]=r;return;}
    // Correctness fallback: one bounding rectangle retains every pixel.
    for(uint16_t i=0;i<count;++i) {
        if(rects[i].top<r.top)r.top=rects[i].top;
        if(rects[i].left<r.left)r.left=rects[i].left;
        if(rects[i].bottom>r.bottom)r.bottom=rects[i].bottom;
        if(rects[i].right>r.right)r.right=rects[i].right;
    }
    alignSpan(r);rects[0]=r;count=1;
}

// The inactive buffer holds the frame from two publications ago. A rectangle
// converted for the previous frame must be copied forward unless one rectangle
// converted this frame replaces it completely; conversion follows the copy.
inline bool syncNeeded(const Rect& previous,const Rect* converted,uint16_t count) {
    for(uint16_t i=0;i<count;++i)if(contains(converted[i],previous))return false;
    return true;
}

inline bool viewportValid(const Rect& viewport) {
    return viewport.top>=0 && viewport.left>=0 && viewport.bottom<=480
        && viewport.right<=640 && viewport.bottom-viewport.top==height
        && viewport.right-viewport.left==width;
}

struct Mismatch { uint16_t x,y;uint8_t expected,actual; };
// Independent bitplane decoder for diagnostic builds. Check the entire frame,
// including pixels preserved outside the current dirty rectangles.
inline bool verify(const uint8_t* chunky,const uint8_t* planar,
                   const Rect& viewport,Mismatch& mismatch) {
    if(!chunky || !planar || !viewportValid(viewport))return false;
    for(uint16_t y=0;y<height;++y)for(uint16_t x=0;x<width;++x) {
        uint8_t actual=0;
        for(uint16_t bit=0;bit<planes;++bit)
            if(planar[uint32_t(y)*rowBytes+bit*planeRow+x/8]&(128u>>(x&7)))
                actual|=1u<<bit;
        uint8_t expected=chunky[uint32_t(y+viewport.top)*640+x+viewport.left];
        if(actual!=expected) {mismatch={x,y,expected,actual};return false;}
    }
    return true;
}

// Output coordinates are local to the viewport. Empty intersections are valid
// and produce an empty rectangle. Align relative to the destination, then clip.
inline bool normalize(const Rect& viewport,const Rect& dirty,Rect& local) {
    if(!viewportValid(viewport) || dirty.bottom<dirty.top || dirty.right<dirty.left)return false;
    local={0,0,0,0};
    int32_t top=dirty.top>viewport.top ? dirty.top : viewport.top;
    int32_t left=dirty.left>viewport.left ? dirty.left : viewport.left;
    int32_t bottom=dirty.bottom<viewport.bottom ? dirty.bottom : viewport.bottom;
    int32_t right=dirty.right<viewport.right ? dirty.right : viewport.right;
    if(top>=bottom || left>=right)return true;
    local.top=int16_t(top-viewport.top);local.bottom=int16_t(bottom-viewport.top);
    local.left=int16_t(left-viewport.left);
    local.right=int16_t(right-viewport.left);
    alignSpan(local);
    return true;
}

#ifndef POP2_PLATFORM_AMIGA
// Host oracle only. The game always calls Kalms assembly.
inline bool convert(const uint8_t* chunky,uint8_t* planar,const Rect& viewport,
                    const Rect& dirty,Rect& converted) {
    Rect local;
    if(!chunky || !planar || !normalize(viewport,dirty,local))return false;
    for(int16_t y=local.top;y<local.bottom;++y) {
        const uint8_t* source=chunky+uint32_t(y+viewport.top)*640+viewport.left;
        uint8_t* destination=planar+uint32_t(y)*rowBytes;
        for(int16_t x=local.left;x<local.right;x+=8) {
            for(uint16_t plane=0;plane<planes;++plane) {
                uint8_t bits=0;
                for(uint16_t pixel=0;pixel<8;++pixel)
                    bits=uint8_t((bits<<1)|((source[x+pixel]>>plane)&1));
                destination[plane*planeRow+x/8]=bits;
            }
        }
    }
    converted=local;
    return true;
}
#endif
}
#endif
