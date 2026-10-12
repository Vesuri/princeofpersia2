#include "FileTransfers.h"
namespace FileTransfers {
int32_t eof(const MacFiles& files,int16_t ref,uint32_t& size) {
    const MacFiles::Fork* fork=files.fork(ref);
    if(!fork)return MacFiles::rfNumErr;
    const MacFiles::Entry* entry=files.entry(fork->id);
    if(!entry)return MacFiles::unsupported;
    size=fork->resource ? entry->resourceSize : entry->dataSize;
    return 0;
}
Result read(MacFiles& files,const Backend& backend,int16_t ref,uint16_t mode,int32_t offset,uint8_t* buffer,uint32_t count) {
    const MacFiles::Fork* fork=files.fork(ref);
    if(!fork)return {MacFiles::rfNumErr,0,0};
    if(count>0x7fffffffUL || (count && !buffer))return {MacFiles::paramErr,0,fork->position};
    int32_t error=files.seek(ref,mode,offset);
    if(error)return {error,0,fork->position};
    uint32_t size=0;
    error=eof(files,ref,size);
    if(error)return {error,0,fork->position};
    uint32_t available=fork->position<size ? size-fork->position : 0;
    uint32_t requested=count<available ? count : available,actual=0;
    if(requested) {
        if(!backend.read)return {MacFiles::unsupported,0,fork->position};
        error=backend.read(backend.context,fork->id,fork->resource,fork->position,buffer,requested,actual);
        if(actual>requested)return {MacFiles::unsupported,0,fork->position};
    }
    files.advance(ref,actual);
    // A backend short read before catalog EOF is an I/O failure, not a
    // successful Macintosh EOF: the installed catalog and storage disagree.
    if(!error && actual<requested)error=-36;
    if(!error && actual<count)error=-39;
    return {error,actual,fork->position};
}
Result write(MacFiles& files,const Backend& backend,int16_t ref,uint16_t mode,int32_t offset,const uint8_t* buffer,uint32_t count) {
    const MacFiles::Fork* fork=files.fork(ref);
    if(!fork)return {MacFiles::rfNumErr,0,0};
    if(!fork->writable)return {-61,0,fork->position};
    if(count>0x7fffffffUL || (count && !buffer))return {MacFiles::paramErr,0,fork->position};
    int32_t error=files.seek(ref,mode,offset,true);
    if(error)return {error,0,fork->position};
    if(count>0x7fffffffUL-fork->position)return {MacFiles::paramErr,0,fork->position};
    uint32_t size=0,actual=0;
    error=eof(files,ref,size);
    if(error)return {error,0,fork->position};
    // Zero-byte writes beyond EOF require a backend resize operation. Until
    // that service is wired, stop explicitly instead of claiming persistence.
    if(!count && fork->position>size)return {MacFiles::unsupported,0,fork->position};
    if(count) {
        if(!backend.write)return {MacFiles::unsupported,0,fork->position};
        error=backend.write(backend.context,fork->id,fork->resource,fork->position,buffer,count,actual);
        if(actual>count)return {MacFiles::unsupported,0,fork->position};
    }
    files.advance(ref,actual);
    if(actual) {
        if(fork->position>size) {
            int32_t resized=files.setSize(ref,fork->position,false);
            if(resized)return {MacFiles::unsupported,actual,fork->position};
        }
        files.modified(ref);
    }
    if(!error && actual<count)error=-36;
    return {error,actual,fork->position};
}
}
