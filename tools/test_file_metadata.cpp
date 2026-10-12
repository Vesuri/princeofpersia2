#include <cassert>
#include <cstring>
#include <cstdio>
#include "../src/mac/FileMetadata.h"
int main() {
    const uint8_t finder[]={0x54,0x45,0x53,0x54,0x41,0x49,0x54,0x44,0x04,0,0,0x12,0,0x34,0,0};
    FileMetadata::Record info={};memcpy(info.finder,finder,16);info.created=0xabcd0102;info.modified=0xabcd0304;
    uint8_t storage[34]={};FileMetadata::encode(info,storage+1); // Unaligned native input is valid.
    assert(!memcmp(storage+1,"AFI1",4) && !memcmp(storage+5,finder,16));
    assert(storage[21]==0xab && storage[22]==0xcd && storage[23]==1 && storage[24]==2);
    FileMetadata::Record decoded={};assert(FileMetadata::decode(storage+1,32,decoded));
    assert(!memcmp(decoded.finder,finder,16) && decoded.created==info.created && decoded.modified==info.modified);
    for(uint32_t bit=0;bit<256;++bit) {
        uint8_t damaged[32];memcpy(damaged,storage+1,32);damaged[bit/8]^=1<<(bit%8);
        decoded.created=0xdeadbeef;
        assert(!FileMetadata::decode(damaged,32,decoded) && decoded.created==0xdeadbeef);
    }
    for(uint32_t size=0;size<34;++size)if(size!=32)assert(!FileMetadata::decode(storage+1,size,decoded));
    assert(!FileMetadata::decode(0,32,decoded));
    puts("PASS file-metadata: Finder bytes/dates, endian/unaligned, corruption/length rejection, atomic decode");
}
