extern "C" {
#include <string.h>
}
#include "FileWriteBuffer.h"
void FileWriteBuffer::clear() {
    for(uint16_t i=0;i<maxPages;++i)if(pages_[i].bytes) {
        release_(pages_[i].bytes,pageBytes);pages_[i].bytes=0;
    }
    size_=sourceLimit_=0;dirty_=false;reader_=0;context_=0;allocate_=0;release_=0;
}
int32_t FileWriteBuffer::bind(uint32_t size,Reader reader,void* context,Allocator allocate,Releaser release) {
    if(reader_ || size>0x7fffffffUL || !reader || !allocate || !release)return -50;
    clear();size_=sourceLimit_=size;reader_=reader;context_=context;allocate_=allocate;release_=release;return 0;
}
const FileWriteBuffer::Page* FileWriteBuffer::page(uint32_t offset) const {
    for(uint16_t i=0;i<maxPages;++i)if(pages_[i].bytes && pages_[i].offset==offset)return &pages_[i];
    return 0;
}
int32_t FileWriteBuffer::original(uint32_t offset,uint8_t* bytes,uint32_t count,uint32_t* completed) const {
    if(completed)*completed=0;
    uint32_t wanted=offset<sourceLimit_ ? sourceLimit_-offset : 0;
    if(wanted>count)wanted=count;
    if(wanted) {
        uint32_t actual=0;int32_t error=reader_(context_,offset,bytes,wanted,actual);
        if(actual>wanted)return -36;
        if(completed)*completed=actual;
        if(error)return error;
        if(actual!=wanted)return -36; // A short backing read must not validate a dirty page.
    }
    if(wanted<count)memset(bytes+wanted,0,count-wanted);
    if(completed)*completed=count;
    return 0;
}
int32_t FileWriteBuffer::read(uint32_t offset,uint8_t* bytes,uint32_t count,uint32_t& actual) const {
    actual=0;
    if(!reader_ || count>0x7fffffffUL || offset>0x7fffffffUL-count || (!bytes && count))return -50;
    if(!count)return 0;
    uint32_t wanted=offset<size_ ? size_-offset : 0;if(wanted>count)wanted=count;
    while(actual<wanted) {
        uint32_t at=offset+actual,base=at&~(uint32_t)(pageBytes-1),within=at-base;
        uint32_t take=pageBytes-within;if(take>wanted-actual)take=wanted-actual;
        const Page* p=page(base);
        if(p)memcpy(bytes+actual,p->bytes+within,take);
        else { uint32_t done=0;int32_t error=original(at,bytes+actual,take,&done);if(error) { actual+=done;return error; } }
        actual+=take;
    }
    return actual<count ? -39 : 0;
}
int32_t FileWriteBuffer::write(uint32_t offset,const uint8_t* bytes,uint32_t count,uint32_t& actual) {
    actual=0;
    if(!reader_ || count>0x7fffffffUL || offset>0x7fffffffUL-count || (!bytes && count))return -50;
    while(actual<count) {
        uint32_t at=offset+actual,base=at&~(uint32_t)(pageBytes-1),within=at-base;
        uint32_t take=pageBytes-within;if(take>count-actual)take=count-actual;
        Page* p=const_cast<Page*>(page(base));
        if(!p) {
            for(uint16_t i=0;i<maxPages;++i)if(!pages_[i].bytes) { p=&pages_[i];break; }
            if(!p)return unsupported;
            uint8_t* block=allocate_(pageBytes);if(!block)return -108;
            int32_t error=take==pageBytes ? 0 : original(base,block,pageBytes);
            if(error) { release_(block,pageBytes);return error; }
            p->offset=base;p->bytes=block;
        }
        memcpy(p->bytes+within,bytes+actual,take);actual+=take;
        if(offset+actual>size_)size_=offset+actual;
        dirty_=true;
    }
    return 0;
}
int32_t FileWriteBuffer::resize(uint32_t size) {
    if(!reader_ || size>0x7fffffffUL)return -50;
    if(size==size_)return 0;
    if(size<sourceLimit_)sourceLimit_=size;
    for(uint16_t i=0;i<maxPages;++i)if(pages_[i].bytes) {
        Page& p=pages_[i];
        if(p.offset>=size) { release_(p.bytes,pageBytes);p.bytes=0; }
        else if(size-p.offset<pageBytes)memset(p.bytes+(size-p.offset),0,pageBytes-(size-p.offset));
    }
    size_=size;dirty_=true;return 0;
}
int32_t FileWriteBuffer::flush(Writer writer,Resizer resizer,void* context) {
    if(!reader_ || !writer || !resizer)return -50;
    if(!dirty_)return 0;
    uint8_t* zeros=0;
    int32_t error=0;
    for(uint32_t base=0;base<size_;) {
        uint32_t count=size_-base;if(count>pageBytes)count=pageBytes;
        const Page* p=page(base);
        uint32_t start=0;const uint8_t* bytes=p ? p->bytes : 0;
        if(!p && base+count>sourceLimit_) {
            start=sourceLimit_>base ? sourceLimit_-base : 0;
            if(!zeros) { zeros=allocate_(pageBytes);if(!zeros) { error=-108;break; }memset(zeros,0,pageBytes); }
            bytes=zeros;
        }
        if(bytes) {
            uint32_t actual=0;
            error=writer(context,base+start,bytes+start,count-start,actual);
            if(error || actual!=count-start) { if(!error)error=-36;break; }
        }
        base+=count;
    }
    if(zeros)release_(zeros,pageBytes);
    if(!error)error=resizer(context,size_);
    if(error)return error;
    for(uint16_t i=0;i<maxPages;++i)if(pages_[i].bytes) {
        release_(pages_[i].bytes,pageBytes);pages_[i].bytes=0;
    }
    sourceLimit_=size_;dirty_=false;return 0;
}

int32_t FileWriteBuffer::flushWhole(Replacer replace,void* context) {
    if(!reader_ || !replace)return -50;
    if(!dirty_)return 0;
    // Same bounded maximum as the sparse write overlay; no partial publication.
    if(size_>maxPages*pageBytes)return unsupported;
    uint8_t* assembled=allocate_(size_?size_:1);if(!assembled)return -108;
    int32_t error=0;
    for(uint32_t offset=0;offset<size_;) {
        uint32_t take=size_-offset,actual=0;if(take>pageBytes)take=pageBytes;
        error=read(offset,assembled+offset,take,actual);
        if(error || actual!=take) { if(!error)error=-36;break; }
        offset+=take;
    }
    if(!error)error=replace(context,assembled,size_);
    release_(assembled,size_?size_:1);
    if(error)return error;
    for(uint16_t i=0;i<maxPages;++i)if(pages_[i].bytes) {
        release_(pages_[i].bytes,pageBytes);pages_[i].bytes=0;
    }
    sourceLimit_=size_;dirty_=false;return 0;
}
