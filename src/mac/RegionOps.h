#ifndef AMIGAXDEV_REGION_OPS_H
#define AMIGAXDEV_REGION_OPS_H
#include "RegionRows.h"
namespace RegionOps {
// Callers own staging storage, kept off the native supervisor stack. Failed
// encoding may change staging bytes, but must never publish a destination.
struct Writer {
    uint8_t* out;uint16_t capacity,used=10;
    RegionRows::Edges previous{};
    int16_t top=32767,left=32767,bottom=0,right=-32768;
    Writer(uint8_t* p,uint16_t n):out(p),capacity(n) {}
    bool row(int16_t y,const RegionRows::Edges& next) {
        if(previous.count)bottom=y;
        if(next.count) {
            if(top==32767)top=y;
            if(next.x[0]<left)left=next.x[0];
            if(next.x[next.count-1]>right)right=next.x[next.count-1];
        }
        RegionRows::Edges changes=previous;
        for(uint16_t i=0;i<next.count;++i)
            if(!RegionRows::toggle(changes,next.x[i]))return false;
        if(changes.count) {
            if(y==32767 || used+changes.count*2+6>capacity)return false;
            WindowGeometry::word(out+used,uint16_t(y));used+=2;
            for(uint16_t i=0;i<changes.count;++i) {
                if(changes.x[i]==32767)return false;
                WindowGeometry::word(out+used,uint16_t(changes.x[i]));used+=2;
            }
            WindowGeometry::word(out+used,32767);used+=2;
        }
        previous=next;return true;
    }
    bool finish(uint16_t& size) {
        size=0;if(!out || capacity<10 || previous.count)return false;
        if(top==32767) {used=10;top=left=bottom=right=0;}
        else {if(used+2>capacity)return false;WindowGeometry::word(out+used,32767);used+=2;if(used==28)used=10;}
        WindowGeometry::word(out,used);WindowGeometry::word(out+2,uint16_t(top));
        WindowGeometry::word(out+4,uint16_t(left));WindowGeometry::word(out+6,uint16_t(bottom));
        WindowGeometry::word(out+8,uint16_t(right));size=used;return true;
    }
};
enum Operation { Xor, Difference, Intersection
#ifdef AMIGAXDEV_REGION_UNION
    , Union
#endif
};
inline bool spans(const RegionRows::Edges& a,const RegionRows::Edges& b,
                  Operation op,RegionRows::Edges& out) {
    out.count=0;uint16_t i=0,j=0;bool inA=false,inB=false,previous=false;
    while(i<a.count || j<b.count) {
        int16_t x=j==b.count || (i<a.count && a.x[i]<b.x[j]) ? a.x[i] : b.x[j];
        if(i<a.count && a.x[i]==x) {inA=!inA;++i;}
        if(j<b.count && b.x[j]==x) {inB=!inB;++j;}
#ifdef AMIGAXDEV_REGION_UNION
        bool next=op==Xor ? inA!=inB : op==Difference ? inA && !inB : op==Union ? inA || inB : inA && inB;
#else
        bool next=op==Xor ? inA!=inB : op==Difference ? inA && !inB : inA && inB;
#endif
        if(next!=previous) {
            if(out.count==16)return false;
            out.x[out.count++]=x;previous=next;
        }
    }
    return !previous;
}
inline int32_t nextLine(const RegionRows::Cursor& c,int32_t y) {
    if(c.size==10) {
        int16_t top=RegionRows::get(c.region+2),bottom=RegionRows::get(c.region+6);
        return y<top ? top : y<bottom ? bottom : 32767;
    }
    return RegionRows::get(c.region+c.at);
}
inline bool combine(const uint8_t* a,uint16_t aBytes,const uint8_t* b,uint16_t bBytes,
                    Operation op,uint8_t* scratch,uint16_t capacity,uint16_t& size) {
    size=0;if(!scratch || capacity<10)return false;
    RegionRows::Cursor ca,cb;
    if(!ca.begin(a,aBytes) || !cb.begin(b,bBytes))return false;
    Writer writer(scratch,capacity);
    int32_t y=-32769;
    for(;;) {
        int32_t ay=nextLine(ca,y),by=nextLine(cb,y);y=ay<by?ay:by;
        if(y==32767)break;
        if(!ca.advance(int16_t(y)) || !cb.advance(int16_t(y)))return false;
        RegionRows::Edges next{};
        if(!spans(ca.edges,cb.edges,op,next) || !writer.row(int16_t(y),next))return false;
    }
    return writer.finish(size);
}
// MapPt rounds the magnitude, including negative half values away from zero.
// Keep the product unsigned: two 16-bit coordinate distances fit in 32 bits.
inline bool mapped(int16_t value,int16_t from,int16_t to,uint32_t sourceExtent,
                   uint32_t targetExtent,int16_t& result) {
    int32_t offset=int32_t(value)-from;
    uint32_t magnitude=uint32_t(offset<0?-offset:offset);
    magnitude=(magnitude*targetExtent+sourceExtent/2)/sourceExtent;
    if(magnitude>65535)return false;
    int32_t position=int32_t(to)+(offset<0?-int32_t(magnitude):int32_t(magnitude));
    if(position<-32768 || position>32767)return false;
    result=int16_t(position);return true;
}
inline bool map(const uint8_t* region,uint16_t bytes,const uint8_t* from,const uint8_t* to,
                uint8_t* scratch,uint16_t capacity,uint16_t& size) {
    size=0;if(!from || !to || !scratch || capacity<10)return false;
    RegionRows::Cursor cursor;if(!cursor.begin(region,bytes))return false;
    int16_t source[4],target[4];
    for(uint16_t i=0;i<4;++i) {source[i]=RegionRows::get(from+2*i);target[i]=RegionRows::get(to+2*i);}
    int32_t sh=int32_t(source[2])-source[0],sw=int32_t(source[3])-source[1];
    int32_t th=int32_t(target[2])-target[0],tw=int32_t(target[3])-target[1];
    if(sh<=0 || sw<=0 || th<0 || tw<0)return false;
    Writer writer(scratch,capacity);
    if(!th || !tw || RegionRows::get(region+2)==RegionRows::get(region+6)
       || RegionRows::get(region+4)==RegionRows::get(region+8))return writer.finish(size);
    if(cursor.size==10) {
        int16_t bounds[4];
        for(uint16_t i=0;i<4;++i) {
            uint16_t axis=i&1;
            if(!mapped(RegionRows::get(region+2+i*2),source[axis],target[axis],
                       axis?sw:sh,axis?tw:th,bounds[i]))return false;
        }
        if(bounds[0]==bounds[2] || bounds[1]==bounds[3])return writer.finish(size);
        WindowGeometry::word(scratch,10);
        for(uint16_t i=0;i<4;++i)WindowGeometry::word(scratch+2+i*2,uint16_t(bounds[i]));
        size=10;return true;
    }
    int32_t y=-32769;int16_t pendingY=0;bool pending=false;
    RegionRows::Edges pendingEdges{};
    for(;;) {
        y=nextLine(cursor,y);if(y==32767)break;
        int16_t mappedY;
        if(!mapped(int16_t(y),source[0],target[0],sh,th,mappedY) || mappedY==32767
           || !cursor.advance(int16_t(y)))return false;
        if(pending && mappedY!=pendingY && !writer.row(pendingY,pendingEdges))return false;
        pending=true;pendingY=mappedY;pendingEdges.count=0;
        for(uint16_t i=0;i<cursor.edges.count;++i) {
            int16_t x;
            if(!mapped(cursor.edges.x[i],source[1],target[1],sw,tw,x) || x==32767
               || !RegionRows::toggle(pendingEdges,x))return false;
        }
    }
    if(pending && !writer.row(pendingY,pendingEdges))return false;
    return writer.finish(size);
}
// Reached FrameOval recording uses circular bounds. Pixel centres are tested
// with integer arithmetic. Non-circular ellipses remain unsupported pending
// exact QuickDraw rounding coverage, rather than silently approximated.
inline bool circle(const uint8_t* rect,uint8_t* scratch,uint16_t capacity,uint16_t& size) {
    size=0;if(!rect || !scratch || capacity<10)return false;
    int32_t top=RegionRows::get(rect),left=RegionRows::get(rect+2);
    int32_t bottom=RegionRows::get(rect+4),right=RegionRows::get(rect+6);
    Writer writer(scratch,capacity);
    if(top>=bottom || left>=right)return writer.finish(size);
    int32_t diameter=right-left;
    if(diameter!=bottom-top || diameter>32766 || bottom>=32767 || right>=32767)return false;
    const int32_t square=diameter*diameter;
    for(int32_t y=top;y<=bottom;++y) {
        RegionRows::Edges next{};
        if(y<bottom) {
            int32_t dy=2*(y-top)+1-diameter;
            int32_t lo=0,hi=diameter/2;
            while(lo<hi) {
                int32_t x=(lo+hi)/2,dx=2*x+1-diameter;
                if(dx*dx+dy*dy<=square)hi=x;else lo=x+1;
            }
            int32_t dx=2*lo+1-diameter;
            if(dx*dx+dy*dy<=square) {
                next.count=2;next.x[0]=int16_t(left+lo);next.x[1]=int16_t(right-lo);
            }
        }
        if(!writer.row(int16_t(y),next))return false;
    }
    return writer.finish(size);
}
}
#endif
