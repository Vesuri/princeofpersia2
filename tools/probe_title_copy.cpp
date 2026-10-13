#include "mac/CopyBits8.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>
static std::vector<uint8_t> read(const std::string& name) {
    std::ifstream stream(name, std::ios::binary);
    return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}
int main(int argc,char** argv) {
    if(argc!=2 && argc!=3)return 1;
    std::string prefix=std::string(argv[1])+"/integration-"+(argc==3?argv[2]:"1")+"-";
    auto source=read(prefix+"before-copy-Source-pixels.bin");
    auto destination=read(prefix+"before-copy-Destination-pixels.bin");
    auto sm=read(prefix+"before-copy-Source-map.bin"),dm=read(prefix+"before-copy-Destination-map.bin");
    auto from=read(prefix+"copy-from.bin"),to=read(prefix+"copy-to.bin");
    if(sm.size()!=50 || dm.size()!=50 || from.size()!=8 || to.size()!=8)return 2;
    auto current=read(prefix+"before-pixmap.bin"),port=read(prefix+"before-port.bin");
    auto vis=read(prefix+"before-vis.bin"),clip=read(prefix+"before-clip.bin");
    if(current.size()!=50 || port.size()!=108 || vis.size()!=10 || clip.size()!=10)return 4;
    bool currentDestination=current==dm;
    if(currentDestination && (RectBounds::word(vis.data())!=10 || RectBounds::word(clip.data())!=10))return 5;
    uint8_t drawn[8];
    if(!CopyBits8::copy(source.data(),source.size(),RectBounds::word(sm.data()+4)&0x3fff,sm.data()+6,
        destination.data(),destination.size(),RectBounds::word(dm.data()+4)&0x3fff,dm.data()+6,
        from.data(),to.data(),currentDestination?port.data()+16:dm.data()+6,
        currentDestination?vis.data()+2:dm.data()+6,currentDestination?clip.data()+2:dm.data()+6,drawn))return 3;
    std::cout.write(reinterpret_cast<const char*>(destination.data()),destination.size());
}
