#include "OriginalApplication.h"
#include "SHA256.h"
static uint16_t word(const uint8_t* p) { return uint16_t(p[0])<<8|p[1]; }
static uint32_t lng(const uint8_t* p) { return uint32_t(word(p))<<16|word(p+2); }
void OriginalApplication::close() {
    bytes_=0;map_.close();world_={};error_="APPLICATION / NOT OPEN";
}
bool OriginalApplication::fail(const char* error) { close();error_=error;return false; }
bool OriginalApplication::open(const uint8_t* bytes,uint32_t size,const uint8_t expected[32]) {
    close();
    if(!bytes || !expected || size<16 || size>0x7fffffffUL)return fail("APPLICATION / INPUT SIZE");
#if !defined(POP2_PLATFORM_AMIGA) || defined(POP2_VERIFY)
    SHA256 sha;uint8_t digest[32];sha_init(&sha);sha_update(&sha,bytes,size);sha_final(&sha,digest);
    for(unsigned i=0;i<32;++i)if(digest[i]!=expected[i])return fail("APPLICATION / SHA256");
#endif
    ResourceMap::Layout layout;
    if(!ResourceMap::layout(bytes,size,layout) ||
       !map_.open(bytes,size,bytes+layout.mapOffset,layout.mapLength))return fail("APPLICATION / RESOURCE MAP");
    for(uint16_t i=0;i<map_.count();++i) {
        ResourceMap::Entry entry;uint32_t offset;
        if(!map_.entry(i,entry) || !map_.payload(i,lng(bytes+entry.lengthOffset),offset))
            return fail("APPLICATION / RESOURCE EXTENT");
        for(uint16_t j=0;j<i;++j) {
            ResourceMap::Entry previous;map_.entry(j,previous);
            if(entry.type==previous.type && entry.id==previous.id)return fail("APPLICATION / DUPLICATE RESOURCE");
        }
    }
    bytes_=bytes;
    if(!startupLayout())return false;
    error_=0;return true;
}
bool OriginalApplication::resource(uint32_t type,int16_t id,Resource& out) const {
    out={};if(!bytes_)return false;
    for(uint16_t i=0;i<map_.count();++i) {
        ResourceMap::Entry entry;if(!map_.entry(i,entry))return false;
        if(entry.type==type && entry.id==id) {
            uint32_t size=lng(bytes_+entry.lengthOffset),offset;
            if(!map_.payload(i,size,offset))return false;
            out={bytes_+offset,size,entry.attrs};return true;
        }
    }
    return false;
}
bool OriginalApplication::startupLayout() {
    Resource code0,code;
    if(!resource(0x434f4445,0,code0) || code0.size<16)return fail("APPLICATION / CODE 0 HEADER");
    const uint8_t* p=code0.bytes;
    World w={lng(p),lng(p+4),lng(p+8),lng(p+12)};
    if(w.above>0x100000 || w.below>0x100000 || !w.below || (w.below&1) ||
       w.jumpBytes!=code0.size-16 || !w.jumpBytes || (w.jumpBytes&7) ||
       w.jumpOffset>w.above || w.jumpBytes>w.above-w.jumpOffset)
        return fail("APPLICATION / A5 LAYOUT");
    for(uint32_t i=0;i<w.jumpBytes;i+=8) {
        const uint8_t* entry=p+16+i;
        uint16_t segment=word(entry+4),offset=word(entry);
        if(word(entry+2)!=0x3f3c || word(entry+6)!=0xa9f0 || !segment ||
           !resource(0x434f4445,segment,code) || code.size<4 || offset>=code.size-4)
            return fail("APPLICATION / JUMP TABLE");
        // Near-model segment headers describe a contiguous slice of CODE 0.
        uint32_t first=word(code.bytes),count=word(code.bytes+2);
        if((first&0x8007) || !count || first>w.jumpBytes || count>(w.jumpBytes-first)/8 ||
           i<first || i>=first+count*8)return fail("APPLICATION / NEAR SEGMENT HEADER");
    }
    Resource data,zero,drel,strs;
    if(!resource(0x44415441,0,data) || !resource(0x5a45524f,0,zero) ||
       !resource(0x4452454c,0,drel) || !resource(0x53545253,0,strs))return fail("APPLICATION / STARTUP RESOURCE");
    // Validate the streams without expanding them. Original CODE 1 performs
    // expansion and relocation on the target; this is only a bounds check.
    uint32_t d=0,z=0,expanded=0;
    while(expanded<w.below) {
        if(d+2>data.size || expanded+2>w.below)return fail("APPLICATION / DATA STREAM");
        uint16_t v=word(data.bytes+d);d+=2;expanded+=2;
        if(!v) {
            if(z+2>zero.size)return fail("APPLICATION / ZERO STREAM");
            uint16_t run=word(zero.bytes+z);z+=2;
            if(run>w.below-expanded)return fail("APPLICATION / ZERO EXTENT");
            expanded+=run;
        }
    }
    if(d!=data.size || z!=zero.size || (drel.size&1))return fail("APPLICATION / STARTUP STREAM EXTENT");
    for(uint32_t i=0;i<drel.size;i+=2) {
        int32_t offset=(int16_t)(word(drel.bytes+i)&0xfffe);
        if(offset> -4 || offset< -(int32_t)w.below)return fail("APPLICATION / DREL EXTENT");
    }
    world_=w;return true;
}
