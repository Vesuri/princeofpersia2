#include "FileMetadata.h"
namespace FileMetadata {
static uint32_t read(const uint8_t* p) { return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3]; }
static void write(uint8_t* p,uint32_t value) { p[0]=value>>24;p[1]=value>>16;p[2]=value>>8;p[3]=value; }
static uint32_t checksum(const uint8_t* p) {
    uint32_t hash=2166136261UL;
    for(uint16_t i=0;i<28;++i)hash=(hash^p[i])*16777619UL;
    return hash;
}
void encode(const Record& record,uint8_t* out) {
    write(out,0x41464931UL);
    for(uint16_t i=0;i<16;++i)out[4+i]=record.finder[i];
    write(out+20,record.created);write(out+24,record.modified);write(out+28,checksum(out));
}
bool decode(const uint8_t* input,uint32_t length,Record& record) {
    if(!input || length!=bytes || read(input)!=0x41464931UL || read(input+28)!=checksum(input))return false;
    Record valid;
    for(uint16_t i=0;i<16;++i)valid.finder[i]=input[4+i];
    valid.created=read(input+20);valid.modified=read(input+24);record=valid;return true;
}
}
