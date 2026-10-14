#include <cassert>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <vector>
#include "mac/RegionOps.h"
static std::vector<uint8_t> read(const char* path){std::ifstream f(path,std::ios::binary);assert(f);return {std::istreambuf_iterator<char>(f),{}};}
int main(int argc,char** argv){
    assert(argc==4);auto delta=read(argv[1]),before=read(argv[2]),expected=read(argv[3]);assert(delta.size()==4);
    int16_t dv=RegionRows::get(delta.data()),dh=RegionRows::get(delta.data()+2);
    assert(dv!=32767 && dh!=32767);
    uint8_t from[8]={0,0,0,0,0,1,0,1},to[8];
    WindowGeometry::word(to,uint16_t(dv));WindowGeometry::word(to+2,uint16_t(dh));
    WindowGeometry::word(to+4,uint16_t(dv+1));WindowGeometry::word(to+6,uint16_t(dh+1));
    std::vector<uint8_t> actual(32766,0xa5);uint16_t size=0;
    assert(RegionOps::map(before.data(),before.size(),from,to,actual.data(),actual.size(),size));
    actual.resize(size);assert(actual==expected);
}
