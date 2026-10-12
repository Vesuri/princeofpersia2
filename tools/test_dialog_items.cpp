#include "../src/mac/DialogItems.h"
#include <cassert>
#include <cstdio>
#include <vector>
int main() {
    std::vector<uint8_t> bytes(2,0);bytes[1]=2;
    for(unsigned length: {9u,9u,51u}) {
        unsigned pos=bytes.size();bytes.resize(pos+14+length,0);
        bytes[pos+12]=length==51 ? 0x88 : 4;bytes[pos+13]=length;
        if(bytes.size()&1)bytes.push_back(0);
    }
    DialogItems::Item items[3];uint16_t count=0;
    assert(DialogItems::scan(bytes.data(),bytes.size(),items,3,count));assert(count==3);
    assert(items[0].offset==2 && items[1].offset==26 && items[2].offset==50);
    for(unsigned size=0;size<bytes.size();++size)assert(!DialogItems::scan(bytes.data(),size,items,3,count));
    assert(!DialogItems::scan(bytes.data(),bytes.size(),items,2,count));
    bytes.push_back(0);assert(!DialogItems::scan(bytes.data(),bytes.size(),items,3,count));bytes.pop_back();
    bytes[15]=255;assert(!DialogItems::scan(bytes.data(),bytes.size(),items,3,count));
    bytes[0]=255;bytes[1]=255;assert(!DialogItems::scan(bytes.data(),bytes.size(),items,3,count));
    puts("PASS dialog item bounds: complete list, every truncation, count, length and trailing bytes");
}
