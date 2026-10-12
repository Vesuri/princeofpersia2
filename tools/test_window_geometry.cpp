#include "../src/mac/WindowGeometry.h"
#include "../src/mac/RegionRows.h"
#include <cassert>
#include <cstring>
#include <set>
#include <cstdio>
using WindowGeometry::Rect;
static int word(const unsigned char* p) {return int16_t(uint16_t(p[0])<<8|p[1]);}
static bool same(Rect a,Rect b) {return a.top==b.top && a.left==b.left && a.bottom==b.bottom && a.right==b.right;}
static bool contains(const unsigned char* data,int x,int y) {
    if(word(data)==10)return word(data+2)<=y && y<word(data+6) && word(data+4)<=x && x<word(data+8);
    std::set<int> crossings;
    int at=10;
    while(word(data+at)!=32767 && word(data+at)<=y) {
        at+=2;
        while(word(data+at)!=32767) {
            int edge=word(data+at);at+=2;
            if(!crossings.erase(edge))crossings.insert(edge);
        }
        at+=2;assert(crossings.size()%2==0);
    }
    bool inside=false;
    for(int edge:crossings) {if(x<edge)break;inside=!inside;}
    return inside;
}
int main() {
    const Rect frames[]={{-8000,-8000,8000,8000},{168,82,368,402},{150,160,350,480}};
    const Rect ports[]={{0,0,16000,16000},{0,0,200,320},{0,0,200,320}};
    const Rect pixmaps[]={{8000,8000,8480,8640},{-168,-82,312,558},{-150,-160,330,480}};
    for(unsigned i=0;i<3;++i) {
        Rect p{},m{},f{};
        assert(WindowGeometry::layout(frames[i],p,m));
        assert(same(p,ports[i]) && same(m,pixmaps[i]));
        assert(WindowGeometry::frame(p,m,f) && same(f,frames[i]));
    }
    Rect p={1,2,3,4},m=p,keep=p;
    assert(!WindowGeometry::layout({0,0,0,1},p,m) && same(p,keep) && same(m,keep));
    assert(!WindowGeometry::layout({-32768,0,0,1},p,m));
    assert(!WindowGeometry::layout({-32500,0,-32400,1},p,m));
    assert(!WindowGeometry::frame({1,0,200,320},{0,0,480,640},p));
    assert(!WindowGeometry::frame({0,0,200,320},{0,0,479,640},p));
    for(Rect f: {Rect{150,160,350,480},Rect{-10,-5,2,7},Rect{0,0,1,1}}) {
        unsigned char region[44];assert(WindowGeometry::structure4(f,region));
        assert(word(region)==44);
        for(int y=f.top-20;y<=f.bottom+3;++y)for(int x=f.left-2;x<=f.right+3;++x) {
            bool border=x>=f.left-1 && x<f.right+1 && y>=f.top-19 && y<f.bottom+1;
            bool shadow=x>=f.left && x<f.right+2 && y>=f.top-18 && y<f.bottom+2;
            assert(contains(region,x,y)==(border || shadow));
        }
    }
    unsigned char bad[44];std::memset(bad,0xa5,sizeof bad);
    assert(!WindowGeometry::structure4({-32760,0,-32750,10},bad));
    for(auto b:bad)assert(b==0xa5);
    unsigned char gray[76],structure[44],visible[256],local[256];
    RegionRows::desktop(gray);
    assert(WindowGeometry::structure4({150,160,350,480},structure));
    assert(RegionRows::difference(gray,sizeof gray,structure,sizeof structure,{-8000,-8000,8000,8000},visible,sizeof visible));
    assert(RegionRows::difference(gray,sizeof gray,structure,sizeof structure,{-8000,-8000,8000,8000},local,sizeof local,8000,8000));
    assert(word(visible)==108 && word(local)==108);
    const int corners[5]={1,1,2,3,5};
    for(int y=0;y<480;++y) {
        RegionRows::Edges row{};assert(RegionRows::row(visible,sizeof visible,y,row));
        int inset=y>=475 ? corners[y-475] : 0;
        for(int x=0;x<640;++x) {
            bool desktop=y>=20 && x>=inset && x<640-inset;
            bool border=x>=159 && x<481 && y>=131 && y<351;
            bool shadow=x>=160 && x<482 && y>=132 && y<352;
            bool expected=desktop && !(border || shadow);
            assert(contains(visible,x,y)==expected);
            assert(contains(local,x+8000,y+8000)==expected);
            assert(RegionRows::inside(row,x)==expected);
        }
    }
    // Streaming traversal must preserve every pixel, including skipped/repeated
    // rows, translated coordinates and empty rows after the final transition.
    for(int step:{1,7,233})for(int offset:{0,8000}) {
        const uint8_t* region=offset ? local : visible;
        RegionRows::Cursor cursor;
        assert(!cursor.advance(0));
        assert(cursor.begin(region,sizeof visible));
        for(int y=-1;y<=700;y+=step) {
            assert(cursor.advance(int16_t(y+offset)));
            assert(cursor.advance(int16_t(y+offset)));
            for(int x=-1;x<=640;++x)
                assert(RegionRows::inside(cursor.edges,int16_t(x+offset))==contains(region,x+offset,y+offset));
        }
        assert(!cursor.advance(-32768));
    }
    uint8_t rectangle[10]={0,10,0xff,0xfe,0xff,0xfd,0,2,0,4};
    RegionRows::Cursor cursor;assert(cursor.begin(rectangle,sizeof rectangle));
    for(int y=-3;y<=3;++y) {
        assert(cursor.advance(y));
        for(int x=-4;x<=4;++x)
            assert(RegionRows::inside(cursor.edges,x)==(y>=-2 && y<2 && x>=-3 && x<4));
    }
    // Even a malformed suffix beyond the requested first row is rejected.
    uint8_t broken[76];std::memcpy(broken,gray,sizeof gray);broken[75]=0;
    assert(!cursor.begin(broken,sizeof broken));assert(!cursor.advance(20));
    unsigned char small[12];std::memset(small,0xa5,sizeof small);
    assert(!RegionRows::difference(gray,sizeof gray,structure,sizeof structure,{-8000,-8000,8000,8000},small,sizeof small));
    for(auto b:small)assert(b==0xa5);
    unsigned char malformed[76];std::memcpy(malformed,gray,sizeof gray);malformed[0]=0;malformed[1]=75;
    RegionRows::Edges row{};assert(!RegionRows::row(malformed,sizeof malformed,20,row));
    std::puts("PASS window geometry: original resource layouts, inverse coordinates, independent region decode, shadow union, full-screen region subtraction/translation and overflow rejection");
}
