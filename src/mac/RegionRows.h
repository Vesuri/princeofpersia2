#ifndef AMIGAXDEV_REGION_ROWS_H
#define AMIGAXDEV_REGION_ROWS_H
#include "WindowGeometry.h"
namespace RegionRows {
inline int16_t get(const uint8_t* p) {return int16_t(uint16_t(p[0])<<8|p[1]);}
struct Edges { int16_t x[16]; uint16_t count; };
inline bool toggle(Edges& edges,int16_t x) {
    uint16_t at=0;while(at<edges.count && edges.x[at]<x)++at;
    if(at<edges.count && edges.x[at]==x) {
        for(uint16_t i=at;i+1<edges.count;++i)edges.x[i]=edges.x[i+1];
        --edges.count;return true;
    }
    if(edges.count==16)return false;
    for(uint16_t i=edges.count;i>at;--i)edges.x[i]=edges.x[i-1];
    edges.x[at]=x;++edges.count;return true;
}
// Decode QuickDraw's XOR scan-line transitions, retaining no pixel buffer.
inline bool row(const uint8_t* region,uint16_t capacity,int16_t y,Edges& result) {
    if(!region || capacity<10)return false;
    uint16_t size=uint16_t(get(region));
    if(size<10 || size>capacity || (size&1))return false;
    result.count=0;
    if(size==10) {
        int16_t top=get(region+2),left=get(region+4),bottom=get(region+6),right=get(region+8);
        if(top>bottom || left>right)return false;
        if(top<=y && y<bottom && left<right) {result.x[0]=left;result.x[1]=right;result.count=2;}
        return true;
    }
    Edges current{};int32_t lastY=-32769;uint16_t at=10;
    while(at+2<=size) {
        int16_t line=get(region+at);at+=2;
        if(line==32767)return at==size && current.count==0;
        if(line<=lastY)return false;
        lastY=line;int32_t lastX=-32769;bool end=false;
        while(at+2<=size) {
            int16_t x=get(region+at);at+=2;
            if(x==32767) {end=true;break;}
            if(x<=lastX || !toggle(current,x))return false;
            lastX=x;
        }
        if(!end || (current.count&1))return false;
        if(line<=y)result=current;
    }
    return false;
}
// Cache only validation, never pixels. Exact bytes, including size and bounds,
// are the key: a recycled/moved handle cannot reuse stale geometry. Larger
// regions retain the ordinary complete validator. Owned storage stays off the
// small supervisor stack; callers keep the cache alive between copies.
struct ValidationCache {
    static constexpr uint16_t slots=8, maximumBytes=4096;
    struct Entry { uint16_t size=0; uint8_t bytes[maximumBytes]; };
    Entry entries[slots]{};
    uint16_t next=0;
    static bool same(const uint8_t* a,const uint8_t* b,uint16_t size) {
#ifdef __mc68000__
        struct __attribute__((packed,may_alias)) Word { uint32_t value; };
        while(size>=4) {
            uint32_t av,bv;
            __asm__("move.l %1,%0" : "=d"(av) : "m"(*(const Word*)a) : "cc");
            __asm__("move.l %1,%0" : "=d"(bv) : "m"(*(const Word*)b) : "cc");
            if(av!=bv)return false;
            a+=4;b+=4;size-=4;
        }
#endif
        while(size--)if(*a++!=*b++)return false;
        return true;
    }
    __attribute__((noinline)) bool validate(const uint8_t* bytes,uint16_t capacity) {
        if(!bytes || capacity<10)return false;
        const uint16_t size=uint16_t(get(bytes));
        if(size<10 || size>capacity || (size&1))return false;
        // Rectangles are cheaper to validate directly than to cache.
        if(size>10 && size<=maximumBytes)
            for(uint16_t i=0;i<slots;++i)
                if(entries[i].size==size && same(entries[i].bytes,bytes,size))return true;
        Edges checked{};
        if(!row(bytes,capacity,0,checked))return false;
        if(size>10 && size<=maximumBytes) {
            Entry& entry=entries[next];next=(next+1)%slots;
            for(uint16_t i=0;i<size;++i)entry.bytes[i]=bytes[i];
            entry.size=size;
        }
        return true;
    }
};
// Validate the complete stream once, then visit rows in ascending order.
// The caller must keep the region unchanged for the cursor's lifetime.
struct Cursor {
    const uint8_t* region=nullptr;
    uint16_t size=0,at=10;
    int32_t lastY=-32769;
    Edges edges{};
    bool begin(const uint8_t* bytes,uint16_t capacity,ValidationCache* cache=nullptr) {
        Edges checked{};
        region=nullptr;
        if(cache ? !cache->validate(bytes,capacity) : !row(bytes,capacity,0,checked))return false;
        region=bytes;size=uint16_t(get(bytes));at=10;lastY=-32769;edges.count=0;
        return true;
    }
    bool advance(int16_t y) {
        if(!region || y<lastY)return false;
        lastY=y;
        if(size==10) {
            edges.count=0;
            if(get(region+2)<=y && y<get(region+6) && get(region+4)<get(region+8)) {
                edges.x[0]=get(region+4);edges.x[1]=get(region+8);edges.count=2;
            }
            return true;
        }
        while(at+2<=size) {
            int16_t line=get(region+at);
            if(line==32767 || line>y)break;
            at+=2;
            while(at+2<=size) {
                int16_t x=get(region+at);at+=2;
                if(x==32767)break;
                if(!toggle(edges,x))return false;
            }
        }
        return true;
    }
};
// Measured 640x480 desktop: menu strip excluded, five-pixel lower corners.
inline void desktop(uint8_t* out) {
    const int16_t words[38]={76,20,0,480,640,
        20,0,640,32767,475,0,1,639,640,32767,
        477,1,2,638,639,32767,478,2,3,637,638,32767,
        479,3,5,635,637,32767,480,5,635,32767,32767};
    for(uint16_t i=0;i<38;++i)WindowGeometry::word(out+i*2,uint16_t(words[i]));
}
inline bool inside(const Edges& edges,int16_t x) {
    bool value=false;for(uint16_t i=0;i<edges.count && edges.x[i]<=x;++i)value=!value;return value;
}
// Restrict to the logical device, intersect the window content and subtract a
// front window's structure. Emit real QuickDraw region transitions, optionally
// translated to local port coordinates. Unsupported complexity fails loudly.
inline bool difference(const uint8_t* a,uint16_t aSize,const uint8_t* b,uint16_t bSize,
                       const WindowGeometry::Rect& clip,uint8_t* out,uint16_t capacity,
                       int16_t dx=0,int16_t dy=0) {
    if(!out || capacity<12)return false;
    uint8_t encoded[256];uint16_t used=10;Edges previous{};
    int16_t top=480,left=640,bottom=0,right=0;
    for(int16_t y=0;y<=480;++y) {
        Edges first{},second{},next{};
        if(y<480 && clip.top<=y && y<clip.bottom) {
            if(!row(a,aSize,y,first) || !row(b,bSize,y,second))return false;
            bool was=false;
            for(int16_t x=0;x<=640;++x) {
                bool now=x<640 && clip.left<=x && x<clip.right && inside(first,x) && !inside(second,x);
                if(now!=was) {if(!toggle(next,x))return false;was=now;}
            }
        }
        if(next.count) {
            if(y<top)top=y;bottom=y+1;
            if(next.x[0]<left)left=next.x[0];
            if(next.x[next.count-1]>right)right=next.x[next.count-1];
        }
        Edges changes=previous;
        for(uint16_t i=0;i<next.count;++i)if(!toggle(changes,next.x[i]))return false;
        if(changes.count) {
            if(used+4+changes.count*2+2>sizeof encoded)return false;
            int32_t line=int32_t(y)+dy;if(line<-32768 || line>=32767)return false;
            WindowGeometry::word(encoded+used,uint16_t(line));used+=2;
            for(uint16_t i=0;i<changes.count;++i) {
                int32_t x=int32_t(changes.x[i])+dx;if(x<-32768 || x>=32767)return false;
                WindowGeometry::word(encoded+used,uint16_t(x));used+=2;
            }
            WindowGeometry::word(encoded+used,32767);used+=2;
        }
        previous=next;
    }
    if(top==480) {used=10;top=left=bottom=right=0;dx=dy=0;}
    else {WindowGeometry::word(encoded+used,32767);used+=2;}
    int32_t bounds[4]={int32_t(top)+dy,int32_t(left)+dx,int32_t(bottom)+dy,int32_t(right)+dx};
    for(uint16_t i=0;i<4;++i)if(!WindowGeometry::wordRange(bounds[i]))return false;
    if(used>capacity)return false;
    WindowGeometry::word(encoded,used);
    for(uint16_t i=0;i<4;++i)WindowGeometry::word(encoded+2+i*2,uint16_t(bounds[i]));
    for(uint16_t i=0;i<used;++i) {volatile uint8_t value=encoded[i];out[i]=value;}
    return true;
}
}
#endif
