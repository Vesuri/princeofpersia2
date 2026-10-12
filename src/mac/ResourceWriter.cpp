#include "ResourceWriter.h"
static void put16(uint8_t* p,uint16_t v) { p[0]=v>>8;p[1]=v; }
static void put32(uint8_t* p,uint32_t v) { put16(p,v>>16);put16(p+2,v); }
static int32_t writeExact(const ResourceWriter::Sink& sink,uint32_t at,const uint8_t* bytes,uint32_t size) {
    while(size) {
        uint32_t n=size<ResourceForks::chunkBytes ? size : ResourceForks::chunkBytes,actual=0;
        int32_t error=sink.write(sink.context,at,bytes,n,actual);
        if(error)return error;if(actual!=n)return -39;
        at+=n;bytes+=n;size-=n;
    }
    return 0;
}
struct Position { uint32_t data;uint16_t name,type; };
struct WriterLayout {
    Position positions[ResourceForks::kMaximumResources];
    uint32_t types[ResourceForks::kMaximumResources];
    uint16_t typeCount=0;uint32_t dataBytes=0,namesOffset=0,mapBytes=0,mapOffset=0,total=0;
};
static int32_t prepare(const ResourceWriter::Entry* entries,uint16_t count,WriterLayout& layout) {
    if(count>ResourceForks::kMaximumResources || (count && !entries))return -50;
    uint16_t& typeCount=layout.typeCount;uint32_t& dataBytes=layout.dataBytes;uint32_t nameBytes=0;
    auto* positions=layout.positions;auto* types=layout.types;
    for(uint16_t i=0;i<count;++i) {
        const ResourceWriter::Entry& e=entries[i];
        if(!e.source.read || e.offset>e.source.size || e.size>e.source.size-e.offset || (!e.name && e.nameLength)
            || dataBytes>0xffffff || e.size>0xffffffffUL-dataBytes-4)return -50;
        uint16_t t=0;while(t<typeCount && types[t]!=e.type)++t;
        if(t==typeCount)types[typeCount++]=e.type;
        positions[i]={dataBytes,0xffff,t};dataBytes+=4+e.size;
        if(e.name) {
            if(nameBytes>=0xffff)return -50;
            positions[i].name=nameBytes;nameBytes+=1+e.nameLength;
        }
    }
    uint32_t namesOffset=30+8*typeCount+12*count,mapBytes=namesOffset+nameBytes;
    if(namesOffset>0xffff || mapBytes>ResourceForks::maximumMapBytes || dataBytes>0xffffffffUL-257-mapBytes)return -50;
    layout.namesOffset=namesOffset;layout.mapBytes=mapBytes;
    layout.mapOffset=(256+dataBytes+1)&~1UL;layout.total=layout.mapOffset+mapBytes;return 0;
}

int32_t ResourceWriter::measure(const Entry* entries,uint16_t count,uint32_t& size) {
    WriterLayout layout;int32_t error=prepare(entries,count,layout);if(!error)size=layout.total;return error;
}
int32_t ResourceWriter::serialize(const Entry* entries,uint16_t count,const Sink& sink) {
    if(!sink.begin || !sink.write || !sink.finish)return -50;
    WriterLayout layout;int32_t checked=prepare(entries,count,layout);if(checked)return checked;
    const auto* positions=layout.positions;const auto* types=layout.types;
    const uint16_t typeCount=layout.typeCount;
    const uint32_t dataBytes=layout.dataBytes,namesOffset=layout.namesOffset,mapBytes=layout.mapBytes,mapOffset=layout.mapOffset,total=layout.total;
    uint8_t* map=new uint8_t[mapBytes];uint8_t* buffer=new uint8_t[ResourceForks::chunkBytes];
    if(!map || !buffer) { delete[] map;delete[] buffer;return -108; }
    for(uint32_t i=0;i<mapBytes;++i)map[i]=0;
    for(uint16_t i=0;i<256;++i)buffer[i]=0;
    put32(buffer,256);put32(buffer+4,mapOffset);put32(buffer+8,dataBytes);put32(buffer+12,mapBytes);
    volatile uint8_t* headerCopy=map; // Avoid the known m68k shared-base byte-copy form.
    for(uint16_t i=0;i<16;++i)headerCopy[i]=buffer[i];
    put16(map+24,28);put16(map+26,namesOffset);put16(map+28,typeCount-1);
    uint32_t reference=30+8*typeCount;
    for(uint16_t t=0;t<typeCount;++t) {
        uint8_t* type=map+30+8*t;put32(type,types[t]);put16(type+6,reference-28);uint16_t n=0;
        for(uint16_t i=0;i<count;++i)if(positions[i].type==t) {
            const Entry& e=entries[i];const Position& p=positions[i];uint8_t* r=map+reference;
            put16(r,e.id);put16(r+2,p.name);put32(r+4,(uint32_t)e.attrs<<24|p.data);
            if(e.name) {
                volatile uint8_t* name=map+namesOffset+p.name;name[0]=e.nameLength;
                for(uint16_t j=0;j<e.nameLength;++j)name[j+1]=e.name[j];
            }
            reference+=12;++n;
        }
        put16(type+4,n-1);
    }
    int32_t error=sink.begin(sink.context,total);
    if(!error)error=writeExact(sink,0,buffer,256);
    for(uint16_t i=0;i<count && !error;++i) {
        const Entry& e=entries[i];uint32_t at=256+positions[i].data;
        put32(buffer,e.size);error=writeExact(sink,at,buffer,4);at+=4;
        for(uint32_t pos=0;pos<e.size && !error;) {
            uint32_t n=e.size-pos<ResourceForks::chunkBytes ? e.size-pos : ResourceForks::chunkBytes,actual=0;
            error=e.source.read(e.source.context,e.offset+pos,buffer,n,actual);
            if(!error && actual!=n)error=-39;
            if(!error)error=writeExact(sink,at+pos,buffer,n);pos+=n;
        }
    }
    if(!error && mapOffset>256+dataBytes) { buffer[0]=0;error=writeExact(sink,mapOffset-1,buffer,1); }
    if(!error)error=writeExact(sink,mapOffset,map,mapBytes);
    if(!error)error=sink.finish(sink.context,true);
    if(error) { int32_t aborted=sink.finish(sink.context,false);if(aborted)error=aborted; }
    delete[] buffer;delete[] map;return error;
}
