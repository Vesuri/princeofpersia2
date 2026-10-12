#ifndef POP2_POLYGON_REGION_H
#define POP2_POLYGON_REGION_H
#include "RegionRows.h"
namespace PolygonRegion {
enum { capacity=4096, maxEdges=64 };
struct Edge {
    int16_t x,top,bottom,sign;
    int32_t pixel;
    uint32_t fraction,slope;
    void advance() {
        fraction+=slope&65535;
        pixel+=int32_t(slope>>16)+int32_t(fraction>>16);
        fraction&=65535;
    }
};
// Caller owns the large staging arrays; native callers must keep them off
// the shared supervisor stack. Output remains atomic on validation failure.
struct Scratch { Edge edges[maxEdges]; uint8_t encoded[capacity]; };
// Measured FramePoly inside OpenRgn. Record the contour, ignoring pixel clip
// regions, then emit QuickDraw XOR scanline transitions. No framebuffer exists
// in this operation. Limits and malformed/unclosed records fail atomically.
inline bool encode(const uint8_t* polygon,uint32_t bytes,uint8_t* out,
                   uint16_t outCapacity,uint16_t& resultSize,Scratch& workspace) {
    resultSize=0;
    if(!polygon || !out || bytes<26 || outCapacity<10)return false;
    uint16_t size=uint16_t(RegionRows::get(polygon));
    if(size<26 || size>bytes || (size-10)%4 || (size-10)/4>maxEdges+1)return false;
    if(RegionRows::get(polygon+10)!=RegionRows::get(polygon+size-4)
       || RegionRows::get(polygon+12)!=RegionRows::get(polygon+size-2))return false;
    int16_t minY=32767,maxY=-32768,minX=32767,maxX=-32768;
    for(uint16_t at=10;at<size;at+=4) {
        int16_t y=RegionRows::get(polygon+at),x=RegionRows::get(polygon+at+2);
        if(y==32767 || x==32767)return false; // region stream terminator
        if(y<minY)minY=y;
        if(y>maxY)maxY=y;
        if(x<minX)minX=x;
        if(x>maxX)maxX=x;
    }
    if(RegionRows::get(polygon+2)!=minY || RegionRows::get(polygon+4)!=minX
       || RegionRows::get(polygon+6)!=maxY || RegionRows::get(polygon+8)!=maxX)return false;
    Edge* edges=workspace.edges;uint16_t count=0;
    for(uint16_t at=10;at+4<size;at+=4) {
        int16_t y0=RegionRows::get(polygon+at),x0=RegionRows::get(polygon+at+2);
        int16_t y1=RegionRows::get(polygon+at+4),x1=RegionRows::get(polygon+at+6);
        if(y0==y1)continue;
        if(y0>y1) { int16_t t=y0;y0=y1;y1=t;t=x0;x0=x1;x1=t; }
        int32_t dx=int32_t(x1)-x0,dy=int32_t(y1)-y0;
        uint32_t slope=(uint32_t(dx<0?-dx:dx)<<16)/uint32_t(dy);
        uint32_t start=32768+slope/2;
        // Original steep positive edges advance before the first scanline;
        // shallow/diagonal negative edges use the preceding pixel boundary.
        if(dx>0 && dx<dy)start+=slope;
        // A descending shallow edge owns an exact boundary on the next row,
        // as in Line8. Keep the fractional bias through subsequent advances.
        if(dx<0 && -dx>dy)--start;
        if(dx<0 && -dx>=dy)start-=65536;
        edges[count++]={x0,y0,y1,int16_t(dx<0?-1:1),int32_t(start>>16),start&65535,slope};
    }
    uint8_t* encoded=workspace.encoded;uint16_t used=10;
    RegionRows::Edges previous{};
    int16_t top=32767,left=32767,bottom=0,right=-32768;
    for(int32_t y=minY;y<=maxY;++y) {
        RegionRows::Edges next{};
        for(uint16_t i=0;i<count;++i) {
            Edge& edge=edges[i];
            if(y<edge.top || y>=edge.bottom)continue;
            int32_t x=int32_t(edge.x)+edge.sign*edge.pixel;
            if(x<-32768 || x>=32767 || !RegionRows::toggle(next,int16_t(x)))return false;
            edge.advance();
        }
        if(next.count&1)return false;
        if(next.count) {
            if(y<top)top=int16_t(y);
            bottom=int16_t(y+1);
            if(next.x[0]<left)left=next.x[0];
            if(next.x[next.count-1]>right)right=next.x[next.count-1];
        }
        RegionRows::Edges changes=previous;
        for(uint16_t i=0;i<next.count;++i)if(!RegionRows::toggle(changes,next.x[i]))return false;
        if(changes.count) {
            if(used+4+changes.count*2+2>capacity)return false;
            WindowGeometry::word(encoded+used,uint16_t(y));used+=2;
            for(uint16_t i=0;i<changes.count;++i) {WindowGeometry::word(encoded+used,uint16_t(changes.x[i]));used+=2;}
            WindowGeometry::word(encoded+used,32767);used+=2;
        }
        previous=next;
    }
    if(previous.count)return false;
    if(top==32767) { used=10;top=left=bottom=right=0; }
    else {
        WindowGeometry::word(encoded+used,32767);used+=2;
        if(used==28)used=10; // canonical rectangle: two identical transition pairs
    }
    if(used>outCapacity)return false;
    WindowGeometry::word(encoded,used);WindowGeometry::word(encoded+2,uint16_t(top));
    WindowGeometry::word(encoded+4,uint16_t(left));WindowGeometry::word(encoded+6,uint16_t(bottom));
    WindowGeometry::word(encoded+8,uint16_t(right));
    for(uint16_t i=0;i<used;++i)out[i]=encoded[i];
    resultSize=used;return true;
}
}
#endif
