// Host validation of the runtime reader against independently parsed originals.
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <vector>
#include "mac/ResourceDirectory.h"
#include "mac/SHA256.h"
struct Source {
    std::vector<uint8_t> bytes;
    static int32_t read(void* context,uint32_t offset,uint8_t* output,uint32_t count,uint32_t& actual) {
        auto& source=*(Source*)context;actual=0;
        if(offset>source.bytes.size() || count>source.bytes.size()-offset)return -39;
        std::copy_n(source.bytes.data()+offset,count,output);actual=count;return 0;
    }
};
int main(int argc,char** argv) {
    if(argc!=2)return 2;
    std::ifstream input(argv[1],std::ios::binary);if(!input)return 3;
    Source source;source.bytes.assign(std::istreambuf_iterator<char>(input),{});
    ResourceDirectory directory;
    if(directory.open(1,{&source,(uint32_t)source.bytes.size(),Source::read},false))return 4;
    for(uint16_t i=0;i<directory.count(1);++i) {
        ResourceDirectory::View view;
        if(!directory.at(1,i,view))return 5;
        SHA256 hash;sha_init(&hash);uint8_t buffer[65536];
        for(uint32_t offset=0;offset<view.entry.size;) {
            uint32_t count=std::min(uint32_t(sizeof(buffer)),view.entry.size-offset);
            if(directory.read(view.identity,offset,buffer,count))return 6;
            sha_update(&hash,buffer,count);offset+=count;
        }
        uint8_t digest[32];sha_final(&hash,digest);
        std::printf("%08x %d %u %u ",view.entry.type,view.entry.id,view.entry.attrs,view.entry.size);
        for(uint8_t byte:digest)std::printf("%02x",byte);
        std::printf(" ");
        if(!view.entry.nameLength)std::printf("-");
        for(unsigned n=0;n<view.entry.nameLength;++n)std::printf("%02x",view.entry.name[n]);
        std::printf("\n");
    }
    if(directory.close(1) || directory.active(1))return 7;
    return 0;
}
