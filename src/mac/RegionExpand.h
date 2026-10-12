#ifndef POP2_REGION_EXPAND_H
#define POP2_REGION_EXPAND_H
#include "RegionRows.h"
namespace RegionExpand {
enum { capacity=4096 };
inline bool unite(RegionRows::Edges& edges,int16_t left,int16_t right) {
    uint16_t first=0;
    while(first<edges.count && edges.x[first+1]<left)first+=2;
    uint16_t last=first;
    while(last<edges.count && edges.x[last]<=right) {
        if(edges.x[last]<left)left=edges.x[last];
        if(edges.x[last+1]>right)right=edges.x[last+1];
        last+=2;
    }
    uint16_t count=edges.count-(last-first)+2;
    if(count>16)return false;
    if(last==first)for(uint16_t i=edges.count;i>last;--i)edges.x[i+1]=edges.x[i-1];
    else for(uint16_t i=last;i<edges.count;++i)edges.x[first+2+i-last]=edges.x[i];
    edges.x[first]=left;edges.x[first+1]=right;edges.count=count;return true;
}
// Measured InsetRgn(-1,-1): union horizontally expanded spans from three
// adjacent rows. Output is staged locally so malformed/oversized input cannot
// partially publish a region. Other inset distances are not claimed here.
inline bool one(const uint8_t* region,uint32_t bytes,uint8_t* out,uint16_t limit,uint16_t& size) {
    size=0;
    if(!region || !out || bytes<10 || limit<10)return false;
    uint16_t extent=uint16_t(RegionRows::get(region));
    if(extent<10 || extent>bytes || extent>capacity || (extent&1))return false;
    int16_t top=RegionRows::get(region+2),left=RegionRows::get(region+4);
    int16_t bottom=RegionRows::get(region+6),right=RegionRows::get(region+8);
    if(top>bottom || left>right)return false;
    uint8_t encoded[capacity];uint16_t used=10;
    if(top==bottom || left==right) {
        if(extent!=10)return false;
        top=left=bottom=right=0;
    } else {
        if(top<=-32768 || left<=-32768 || bottom>=32766 || right>=32766)return false;
        if(extent==10) {--top;--left;++bottom;++right;}
        else {
            // Each neighbour advances monotonically as the output row moves
            // down. Validate once per cursor, then consume each transition
            // once instead of decoding the complete region for every row.
            RegionRows::Cursor rows[3];
            for(uint16_t i=0;i<3;++i)if(!rows[i].begin(region,extent))return false;
            RegionRows::Edges previous{};
            int16_t resultTop=32767,resultLeft=32767,resultBottom=0,resultRight=-32768;
            for(int32_t y=int32_t(top)-1;y<=int32_t(bottom)+1;++y) {
                RegionRows::Edges next{};
                for(int32_t source=y-1;source<=y+1;++source) {
                    if(source<top || source>=bottom)continue;
                    RegionRows::Cursor& cursor=rows[source-y+1];
                    if(!cursor.advance(int16_t(source)))return false;
                    const RegionRows::Edges& row=cursor.edges;
                    for(uint16_t i=0;i<row.count;i+=2) {
                        if(row.x[i]<=-32768 || row.x[i+1]>=32766
                           || !unite(next,int16_t(row.x[i]-1),int16_t(row.x[i+1]+1)))return false;
                    }
                }
                if(next.count) {
                    if(y<resultTop)resultTop=int16_t(y);
                    resultBottom=int16_t(y+1);
                    if(next.x[0]<resultLeft)resultLeft=next.x[0];
                    if(next.x[next.count-1]>resultRight)resultRight=next.x[next.count-1];
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
            if(resultTop==32767) {used=10;resultTop=resultLeft=resultBottom=resultRight=0;}
            else {WindowGeometry::word(encoded+used,32767);used+=2;if(used==28)used=10;}
            top=resultTop;left=resultLeft;bottom=resultBottom;right=resultRight;
        }
    }
    if(used>limit)return false;
    WindowGeometry::word(encoded,used);WindowGeometry::word(encoded+2,uint16_t(top));
    WindowGeometry::word(encoded+4,uint16_t(left));WindowGeometry::word(encoded+6,uint16_t(bottom));
    WindowGeometry::word(encoded+8,uint16_t(right));
    for(uint16_t i=0;i<used;++i) {volatile uint8_t value=encoded[i];out[i]=value;}
    size=used;return true;
}
}
#endif
