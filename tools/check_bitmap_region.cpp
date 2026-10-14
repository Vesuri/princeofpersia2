#include <cassert>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <vector>
#include "mac/BitmapRegion.h"
static std::vector<uint8_t> read(const char* path){std::ifstream f(path,std::ios::binary);assert(f);return {std::istreambuf_iterator<char>(f),{}};}
int main(int argc,char** argv){
    assert(argc==4);auto map=read(argv[1]),pixels=read(argv[2]),expected=read(argv[3]);assert(map.size()==14);
    std::vector<uint8_t> actual(32766,0xa5);uint16_t size=0;
    assert(BitmapRegion::encode(pixels.data(),pixels.size(),uint16_t(RegionRows::get(map.data()+4)),map.data()+6,actual.data(),actual.size(),size));
    actual.resize(size);assert(actual==expected);
    auto original=actual;
    assert(!BitmapRegion::encode(pixels.data(),pixels.size(),0,map.data()+6,actual.data(),actual.size(),size));
    assert(actual==original);
    assert(!BitmapRegion::encode(pixels.data(),0,uint16_t(RegionRows::get(map.data()+4)),map.data()+6,actual.data(),actual.size(),size));
}
