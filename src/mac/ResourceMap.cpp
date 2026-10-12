#include "ResourceMap.h"
static uint16_t word(const uint8_t* p) { return (uint16_t)p[0]<<8|p[1]; }
static uint32_t lng(const uint8_t* p) { return (uint32_t)word(p)<<16|word(p+2); }
static bool range(uint32_t at,uint32_t length,uint32_t size) { return at<=size && length<=size-at; }
bool ResourceMap::layout(const uint8_t* header,uint32_t size,Layout& out) {
    if(!header || size<16)return false;
    Layout v={lng(header),lng(header+4),lng(header+8),lng(header+12)};
    if(v.dataOffset<16 || v.mapOffset<16 || v.mapLength<30
        || !range(v.dataOffset,v.dataLength,size) || !range(v.mapOffset,v.mapLength,size))return false;
    if(v.dataLength && v.dataOffset<v.mapOffset+v.mapLength && v.mapOffset<v.dataOffset+v.dataLength)return false;
    out=v;return true;
}
void ResourceMap::close() { map_=0;layout_={};count_=0; }
bool ResourceMap::open(const uint8_t* header,uint32_t sourceSize,const uint8_t* map,uint32_t mapSize) {
    close();Layout v;
    if(!layout(header,sourceSize,v) || !map || mapSize!=v.mapLength)return false;
    for(uint16_t i=0;i<16;++i)if(map[i]!=header[i])return false;
    uint32_t types=word(map+24),names=word(map+26);
    if(types<28 || !range(types,2,mapSize) || names>mapSize)return false;
    uint32_t typeCount=(uint16_t)(word(map+types)+1);
    if(typeCount>maximumResources || !range(types+2,typeCount*8,mapSize))return false;
    uint32_t typeEnd=types+2+typeCount*8,total=0;
    if(names<typeEnd)return false;
    for(uint32_t t=0;t<typeCount;++t) {
        const uint8_t* type=map+types+2+t*8;
        uint32_t refs=types+word(type+6),n=(uint16_t)(word(type+4)+1);
        if(!n || n>maximumResources-total || refs<typeEnd || !range(refs,n*12,names))return false;
        for(uint32_t old=0;old<t;++old) {
            const uint8_t* prior=map+types+2+old*8;
            uint32_t start=types+word(prior+6),end=start+((uint32_t)word(prior+4)+1)*12;
            if(lng(prior)==lng(type) || (refs<end && start<refs+n*12))return false;
        }
        for(uint32_t r=0;r<n;++r) {
            const uint8_t* ref=map+refs+r*12;
            uint32_t relative=(uint32_t)ref[5]<<16|word(ref+6);
            if(!range(relative,4,v.dataLength))return false;
            uint16_t name=word(ref+2);
            if(name!=0xffff) {
                uint32_t pos=names+name;
                if(!range(pos,1,mapSize) || !range(pos+1,map[pos],mapSize))return false;
            }
        }
        total+=n;
    }
    map_=map;layout_=v;count_=total;return true;
}
bool ResourceMap::entry(uint16_t index,Entry& out) const {
    if(!map_ || index>=count_)return false;
    uint32_t types=word(map_+24),names=word(map_+26),typeCount=(uint16_t)(word(map_+types)+1);
    for(uint32_t t=0;t<typeCount;++t) {
        const uint8_t* type=map_+types+2+t*8;uint32_t n=(uint32_t)word(type+4)+1;
        if(index>=n) { index-=n;continue; }
        const uint8_t* ref=map_+types+word(type+6)+index*12;
        uint16_t name=word(ref+2);const uint8_t* text=name==0xffff ? 0 : map_+names+name;
        out={lng(type),layout_.dataOffset+((uint32_t)ref[5]<<16|word(ref+6)),(int16_t)word(ref),ref[4],(uint8_t)(text ? text[0] : 0),text ? text+1 : 0};
        return true;
    }
    return false;
}
bool ResourceMap::payload(uint16_t index,uint32_t length,uint32_t& offset) const {
    Entry e;if(!entry(index,e))return false;
    uint32_t relative=e.lengthOffset-layout_.dataOffset+4;
    if(!range(relative,length,layout_.dataLength))return false;
    offset=e.lengthOffset+4;return true;
}
