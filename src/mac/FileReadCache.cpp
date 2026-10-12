#include "FileReadCache.h"
void FileReadCache::bind(uint8_t* buffer,uint32_t size,Reader reader,void* context) {
    buffer_=buffer;size_=size;reader_=reader;context_=context;start_=valid_=0;
}
int32_t FileReadCache::read(uint32_t offset,uint8_t* out,uint32_t bytes,uint32_t& actual) {
    actual=0;
    if(!reader_ || !buffer_ || (!out && bytes) || bytes>0x7fffffffUL)return -50;
    if(!bytes)return 0;
    if(offset>=size_)return -39;
    uint32_t wanted=bytes;
    if(wanted>size_-offset)wanted=size_-offset;
    if(bytes>capacity) {
        // One backend transaction; DOS splits its Read calls inside one OS window.
        int32_t error=reader_(context_,offset,out,wanted,actual);
        if(actual>wanted) { actual=0;return -36; }
        return error ? error : actual<bytes ? -39 : 0;
    }
    while(actual<wanted) {
        uint32_t position=offset+actual;
        if(position>=start_ && position-start_<valid_) {
            uint32_t count=valid_-(position-start_);
            if(count>wanted-actual)count=wanted-actual;
            for(uint32_t i=0;i<count;++i)out[actual+i]=buffer_[position-start_+i];
            actual+=count;continue;
        }
        uint32_t count=size_-position;
        if(count>capacity)count=capacity;
        valid_=0;start_=position;
        uint32_t got=0;
        int32_t error=reader_(context_,position,buffer_,count,got);
        if(got>count)return -36;
        if(error || got<count) {
            // A failed fill is never reused. Preserve only bytes actually received.
            uint32_t copied=got;
            if(copied>wanted-actual)copied=wanted-actual;
            for(uint32_t i=0;i<copied;++i)out[actual+i]=buffer_[i];
            actual+=copied;
            return error ? error : actual<bytes ? -39 : 0;
        }
        valid_=got;
    }
    return actual<bytes ? -39 : 0;
}
