// Replay a captured original unmasked self-copy against the shared helper.
#include "mac/CopyBits8.h"
#include <cassert>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
static std::vector<uint8_t> read(const std::string& path) {
    std::ifstream f(path,std::ios::binary);assert(f.good());
    return {std::istreambuf_iterator<char>(f),{}};
}
int main(int argc,char** argv) {
    assert(argc==2);std::string p=argv[1];
    auto pixels=read(p+"-before.bin"),expected=read(p+"-after.bin");
    auto map=read(p+"-map.bin"),from=read(p+"-from.bin"),to=read(p+"-to.bin"),args=read(p+"-args.bin");
    assert(map.size()==50 && from.size()==8 && to.size()==8 && args.size()==22);
    assert(pixels.size()==expected.size());for(unsigned i=0;i<6;++i)assert(args[i]==0);
    uint8_t drawn[8];auto bounds=map.data()+6;auto stride=RectBounds::word(map.data()+4)&0x3fff;
    assert(CopyBits8::copy(pixels.data(),pixels.size(),stride,bounds,pixels.data(),pixels.size(),stride,bounds,
                          from.data(),to.data(),bounds,bounds,bounds,drawn));
    assert(pixels==expected);
}
