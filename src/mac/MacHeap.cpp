#ifndef __AMIGA__
// The cross build force-includes SASCCompat, whose typedefs are the native ABI.
#ifndef __mc68000__
#include <cstdint>
#endif
#endif
#include "MacHeap.h"
#include "FillRect8.h"

static void heapWrite32(uint8_t* at, uint32_t value)
{
    at[0]=value>>24; at[1]=value>>16; at[2]=value>>8; at[3]=value;
}
// unsigned long holds a pointer on both supported builds (Amiga LP32 / host LP64).
static uint32_t heapAddress(const void* p) { return (uint32_t)(unsigned long)p; }

uint32_t MacHeap::physical(uint32_t logical)
{
    if (logical > 0x7fffffffUL-blockBytes-7) return 0;
    uint32_t bytes=(logical+blockBytes+7)&~7UL;
    return bytes<minimumBlock ? minimumBlock : bytes;
}
void MacHeap::moveBytes(uint8_t* dst, const uint8_t* src, uint32_t bytes)
{
    if (dst!=src && bytes) moved_=true;
    if (dst<src) for (uint32_t i=0;i<bytes;++i) dst[i]=src[i];
    else if (dst>src) while (bytes) { --bytes;dst[bytes]=src[bytes]; }
}
void MacHeap::reverseWords(uint8_t* first, uint8_t* last)
{
    // MoveHHi rotates whole physical blocks (eight-byte aligned spans). Three
    // reversals of longword order produce the same rotation without reversing
    // bytes within each word. may_alias permits access to arbitrary payloads.
    typedef uint32_t HeapWord __attribute__((__may_alias__));
    HeapWord* lo=(HeapWord*)first;
    HeapWord* hi=(HeapWord*)last;
    if(lo<hi)moved_=true;
    while(lo<hi) { --hi;if(lo>=hi)break;HeapWord v=*lo;*lo++=*hi;*hi=v; }
}
void MacHeap::reset()
{
    arena_=0;bytes_=end_=freeBytes_=0;error_=0;freeMasters_=0;
    masterBlockCount_=0;masterBlocksOverflow_=false;
    cachedMasterStart_=cachedMasterLength_=0;
    masterRanges_.clear();searchStart_=headerBytes;
}
bool MacHeap::init(uint8_t* arena, uint32_t bytes, uint16_t masters)
{
    reset();
    if (!arena || (unsigned long)arena%8 || bytes<1024 || !masters) { error_=paramErr;return false; }
    arena_=arena;bytes_=bytes&~7UL;end_=bytes_-trailerBytes;masters_=masters;
    freeBytes_=end_-headerBytes;
    for (uint32_t i=0;i<headerBytes;++i) arena_[i]=0;
    block(headerBytes)={end_-headerBytes,0,0,freeBlock,0,0};
    return moreMasters()==0;
}
bool MacHeap::owns(const void* address) const
{
    unsigned long p=(unsigned long)address, a=(unsigned long)arena_;
    return arena_ && p>=a && p-a<bytes_;
}
uint8_t* MacHeap::masterFlags(uint32_t off, uint32_t pos) const
{
    const Block& b=block(off);
    uint32_t count=b.owner, start=off+blockBytes, length=count*sizeof(uint8_t*);
    if (pos>=start && pos-start<length && (pos-start)%sizeof(uint8_t*)==0) {
        cachedMasterStart_=start;cachedMasterLength_=length;
        masterRanges_.remember(pos,start,length);
        return masterRanges_.rememberFlags(arena_+pos,arena_+start+length+(pos-start)/sizeof(uint8_t*));
    }
    return 0;
}
// Keep the master-block walk and its register/stack setup off cache hits.
uint8_t* MacHeap::flagsSlow(Handle handle) const
{
    if (!owns(handle)) return 0;
    uint32_t pos=(uint8_t*)handle-arena_;
    if (!masterBlocksOverflow_) {
        for (uint16_t i=0;i<masterBlockCount_;++i)
            if (uint8_t* f=masterFlags(masterBlocks_[i],pos)) return f;
        return 0;
    }
    for (uint32_t off=headerBytes;off<end_;off+=block(off).span)
        if (block(off).kind==masterBlock)
            if (uint8_t* f=masterFlags(off,pos)) return f;
    return 0;
}
bool MacHeap::isHandle(Handle handle) const { uint8_t* f=flags(handle);return f && (*f&1); }
bool MacHeap::isFreeHandleSlot(Handle handle) const { uint8_t* f=flags(handle);return f && !(*f&1); }
// A live handle block is identified by its own header: the block lies on an
// aligned boundary inside the zone and its owner is a master slot that holds
// exactly this data pointer. Stale headers inside free or coalesced space
// cannot pass, because no master pointer refers to them.
inline __attribute__((always_inline)) uint32_t MacHeap::findHandleBlock(const uint8_t* ptr, Handle validated) const
{
    uint32_t pos=ptr-arena_;
    if (pos<headerBytes+blockBytes || (pos&7)) return 0;
    uint32_t off=pos-blockBytes;
    if (off>end_ || end_-off<minimumBlock) return 0;
    const Block& b=block(off);
    if (b.kind!=handleBlock || b.span<minimumBlock || (b.span&7) || b.span>end_-off) return 0;
    if (b.owner>=end_) return 0;
    if(validated) {
        if(b.owner!=uint32_t((uint8_t*)validated-arena_))return 0;
    } else if(!flags((Handle)(arena_+b.owner)))return 0;
    return *(Handle)(arena_+b.owner)==ptr ? off : 0;
}
uint32_t MacHeap::findPtr(const uint8_t* ptr, uint32_t kind) const
{
    if (!owns(ptr)) return 0;
    if (kind==handleBlock) return findHandleBlock(ptr);
    return scanPtr(ptr,kind);
}
uint32_t MacHeap::scanPtr(const uint8_t* ptr, uint32_t kind) const
{
    if (!owns(ptr)) return 0;
    uint32_t pos=ptr-arena_;
    for (uint32_t off=headerBytes;off<end_;off+=block(off).span)
        if (off+blockBytes==pos && block(off).kind==kind) return off;
    return 0;
}
uint32_t MacHeap::freeBytes() const
{
    return freeBytes_;
}
uint32_t MacHeap::largestBlock() const
{
    uint32_t largest=0;
    for(uint32_t off=headerBytes;off<end_;off+=block(off).span)
        if(block(off).kind==freeBlock && block(off).span>largest)largest=block(off).span;
    return largest ? largest-blockBytes : 0;
}
#include "HeapFindSpace.inc"
void MacHeap::split(uint32_t off,uint32_t span,uint32_t logical,uint32_t kind,uint32_t owner)
{
    if(off<searchStart_)searchStart_=off;
    uint32_t stateOffset=kind==handleBlock ? flags((Handle)(arena_+owner))-arena_ : 0;
    uint32_t available=block(off).span;
    if(block(off).kind==freeBlock)freeBytes_-=available;
    if(available-span>=minimumBlock)block(off+span)={available-span,0,0,freeBlock,0,0};
    else span=available;
    freeBytes_+=available-span;
    block(off)={span,logical,owner,kind,stateOffset,0};
}
void MacHeap::coalesce()
{
    searchStart_=end_;
    for(uint32_t off=headerBytes;off<end_;) {
        if(block(off).kind==freeBlock && searchStart_==end_)searchStart_=off;
        uint32_t next=off+block(off).span;
        if(block(off).kind==freeBlock && next<end_ && block(next).kind==freeBlock)
            block(off).span+=block(next).span;
        else off=next;
    }
}
void MacHeap::release(uint32_t off) { freeBytes_+=block(off).span;block(off).kind=freeBlock;block(off).logical=block(off).owner=0;coalesce(); }
bool MacHeap::movable(uint32_t off) const
{
    if(block(off).kind!=handleBlock)return false;
    // Master blocks are pinned. This direct side-state offset stays valid even
    // while compaction is rebuilding the intervening physical block chain.
    return !(arena_[block(off).stateOffset]&0x80);
}
void MacHeap::publish()
{
    if(!arena_)return;
    heapWrite32(arena_+8,heapAddress(freeMasters_));
    heapWrite32(arena_,heapAddress(arena_+end_));
    heapWrite32(arena_+12,freeBytes());
    arena_[20]=masters_>>8;arena_[21]=masters_;
}
uint32_t MacHeap::compact()
{
    searchStart_=headerBytes;
    uint32_t dst=headerBytes;
    for(uint32_t src=headerBytes;src<end_;) {
        Block saved=block(src);uint32_t next=src+saved.span;
        if(saved.kind!=freeBlock) {
            if(movable(src)) {
                if(dst!=src)moveBytes(arena_+dst,arena_+src,saved.span);
                *(Handle)(arena_+saved.owner)=arena_+dst+blockBytes;
                dst+=saved.span;
            } else {
                if(dst<src)block(dst)={src-dst,0,0,freeBlock,0,0};
                dst=next;
            }
        }
        src=next;
    }
    if(dst<end_)block(dst)={end_-dst,0,0,freeBlock,0,0};
    result(0);return largestBlock();
}
void MacHeap::compactUp()
{
    searchStart_=headerBytes;
    // Each interval is bounded by a pointer, master block or locked handle.
    uint32_t start=headerBytes;
    while(start<end_) {
        if(block(start).kind!=freeBlock && !movable(start)) { start+=block(start).span;continue; }
        uint32_t limit=start;
        while(limit<end_ && (block(limit).kind==freeBlock || movable(limit)))limit+=block(limit).span;
        uint32_t cursor=limit,dst=limit;
        while(cursor>start) {
            uint32_t src=start;
            while(src+block(src).span<cursor)src+=block(src).span;
            Block saved=block(src);
            if(saved.kind!=freeBlock) {
                dst-=saved.span;
                if(dst!=src)moveBytes(arena_+dst,arena_+src,saved.span);
                *(Handle)(arena_+saved.owner)=arena_+dst+blockBytes;
            }
            cursor=src;
        }
        if(dst>start)block(start)={dst-start,0,0,freeBlock,0,0};
        start=limit;
    }
    publish();
}
int16_t MacHeap::purge(uint32_t requested)
{
    compact();
    // Walk stable master blocks; releasing data coalesces the data-block list.
    for(uint32_t off=headerBytes;off<end_ && largestBlock()<requested;) {
        Block b=block(off);
        if(b.kind==masterBlock) {
            Handle handles=(Handle)(arena_+off+blockBytes);
            uint8_t* states=(uint8_t*)(handles+b.owner);
            for(uint32_t i=0;i<b.owner && largestBlock()<requested;++i)
                if((states[i]&0xc1)==0x41 && handles[i])emptyHandle(handles+i);
        }
        // Master blocks never move. Other block boundaries may have coalesced.
        if(b.kind==masterBlock)off+=block(off).span;
        else {
            uint32_t next=headerBytes;
            while(next<end_ && next<=off)next+=block(next).span;
            off=next;
        }
    }
    compact();return result(largestBlock()>=requested ? 0 : memFullErr);
}
uint32_t MacHeap::allocate(uint32_t bytes,uint32_t kind,uint32_t owner)
{
    if(!arena_ || !physical(bytes)) { result(memFullErr);return 0; }
    uint32_t off=findSpace(bytes);
    if(!off) { compact();off=findSpace(bytes); }
    if(!off) { purge(bytes);off=findSpace(bytes); }
    // MEASURED (ref masters-3): the original also moves movable blocks across
    // locked ones to make room. Do so only once compaction and purging fail.
    if(!off) { off=clearLow(bytes);publish(); }
    if(!off) { result(memFullErr);return 0; }
    split(off,physical(bytes),bytes,kind,owner);result(0);return off;
}
// MEASURED (System 7.5.5, ref masters-3): NewPtr and explicit or implicit
// MoreMasters place their block at the lowest run of free and movable blocks
// that covers it, relocating the movable ones elsewhere, even across locked
// blocks. The original picks relocation targets with its own rover; this
// relocates first-fit outside the run.
uint32_t MacHeap::clearLow(uint32_t bytes)
{
    const uint32_t needed=physical(bytes);
    for(uint32_t p=headerBytes;p<end_;p+=block(p).span) {
        if(block(p).kind!=freeBlock && !movable(p))continue;
        uint32_t q=p;
        while(q<end_ && q-p<needed && (block(q).kind==freeBlock || movable(q)))q+=block(q).span;
        if(q-p<needed)continue;
        const uint32_t e=p+needed;
        // Released blocks coalesce, but their stale headers keep the walk valid.
        for(uint32_t m=p;m<q;) {
            const Block saved=block(m);const uint32_t next=m+saved.span;
            if(saved.kind!=freeBlock) {
                const uint32_t span=physical(saved.logical);uint32_t target=0;
                for(uint32_t off=searchStart_;off<end_ && !target;off+=block(off).span) {
                    const uint32_t free=block(off).span;
                    if(block(off).kind!=freeBlock)continue;
                    if(off+free<=p || off>=e) { if(free>=span)target=off; }
                    else if(off+free>e) {
                        // A free block crossing the run's end keeps its upper part usable.
                        const uint32_t cut=e-off>=minimumBlock ? e : off+minimumBlock;
                        if(off+free>=cut+span) {
                            block(off).span=cut-off;block(cut)={off+free-cut,0,0,freeBlock,0,0};target=cut;
                        }
                    }
                }
                if(!target)return 0;
                split(target,span,saved.logical,handleBlock,saved.owner);
                moveBytes(arena_+target+blockBytes,arena_+m+blockBytes,saved.logical);
                *(Handle)(arena_+saved.owner)=arena_+target+blockBytes;
                release(m);
            }
            m=next;
        }
        for(uint32_t off=headerBytes;off<end_;off+=block(off).span)
            if(off<=p && p<off+block(off).span)return block(off).kind==freeBlock ? off : 0;
        return 0;
    }
    return 0;
}
uint32_t MacHeap::allocateLow(uint32_t bytes,uint32_t kind)
{
    if(!arena_ || !physical(bytes)) { result(memFullErr);return 0; }
    uint32_t off=clearLow(bytes);
    if(!off) { compact();off=clearLow(bytes); }
    if(!off) { purge(bytes);off=clearLow(bytes); }
    publish();
    if(!off) { result(memFullErr);return 0; }
    split(off,physical(bytes),bytes,kind,0);result(0);return off;
}
int16_t MacHeap::moreMasters(uint16_t count)
{
    if(!count)count=masters_;
    uint32_t bytes=count*(sizeof(uint8_t*)+1);
    uint32_t off=allocateLow(bytes,ptrBlock);if(!off)return error_;
    for(uint32_t i=0;i<bytes;++i)arena_[off+blockBytes+i]=0;
    block(off).kind=masterBlock;block(off).owner=count;
    if(masterBlockCount_<maxMasterBlocks)masterBlocks_[masterBlockCount_++]=off;
    else masterBlocksOverflow_=true;
    // Splice the new, contiguous slots into the descending-address free
    // chain. Master blocks are pinned, so data moves never change these links.
    Handle handles=(Handle)(arena_+off+blockBytes);
    Handle previous=0,next=freeMasters_;
    while(next && (uint8_t*)next>(uint8_t*)handles) {
        previous=next;next=(Handle)*next;
    }
    handles[0]=(uint8_t*)next;
    for(uint16_t i=1;i<count;++i)handles[i]=(uint8_t*)(handles+i-1);
    if(previous)*previous=(uint8_t*)(handles+count-1);
    else freeMasters_=handles+count-1;
    return result(0);
}
void MacHeap::releaseMaster(Handle handle)
{
    Handle previous=0,next=freeMasters_;
    while(next && (uint8_t*)next>(uint8_t*)handle) {
        previous=next;next=(Handle)*next;
    }
    *handle=(uint8_t*)next;
    if(previous)*previous=(uint8_t*)handle;
    else freeMasters_=handle;
}
uint8_t* MacHeap::newPtr(uint32_t bytes,bool clear)
{
    uint32_t off=allocateLow(bytes,ptrBlock);if(!off)return 0;
    uint8_t* ptr=arena_+off+blockBytes;
    if(clear)FillRect8::fillSpan(ptr,bytes,0);
    return ptr;
}
int16_t MacHeap::disposePtr(uint8_t* ptr)
{
    uint32_t off=findPtr(ptr,ptrBlock);if(!off)return result(memWZErr);
    release(off);return result(0);
}
uint32_t MacHeap::ptrSize(uint8_t* ptr)
{
    uint32_t off=findPtr(ptr,ptrBlock);queryResult(off ? 0 : memWZErr);
    return off ? block(off).logical : 0;
}
#include "HeapResize.inc"
int16_t MacHeap::setPtrSize(uint8_t* ptr,uint32_t bytes)
{
    uint32_t off=findPtr(ptr,ptrBlock);if(!off)return result(memWZErr);
    if(!resizeInPlace(off,bytes)) { compactUp();if(!resizeInPlace(off,bytes))return result(memFullErr); }
    return result(0);
}
MacHeap::Handle MacHeap::newEmptyHandle()
{
    if(!freeMasters_ && moreMasters()!=0)return 0;
    // Preserve the existing lowest-address allocation order. Only free slots
    // are visited; allocated slots and unrelated data blocks are irrelevant.
    Handle previous=0,handle=freeMasters_;
    while(*handle) {previous=handle;handle=(Handle)*handle;}
    if(previous)*previous=0;else freeMasters_=0;
    *flags(handle)=1;*handle=0;result(0);return handle;
}
MacHeap::Handle MacHeap::newHandle(uint32_t bytes,bool clear)
{
    Handle h=newEmptyHandle();if(!h)return 0;
    uint32_t off=allocate(bytes,handleBlock,(uint8_t*)h-arena_);
    if(!off) { *flags(h)=0;releaseMaster(h);result(memFullErr);return 0; }
    *h=arena_+off+blockBytes;
    if(clear)FillRect8::fillSpan(*h,bytes,0);
    result(0);return h;
}
int16_t MacHeap::disposeHandle(Handle h)
{
    // MEASURED: System 7.5.5 DisposeHandle(nil) clears MemError and succeeds.
    if(!h)return result(0);
    if(!isHandle(h))return result(nilHandleErr);
    if(*h) { uint32_t off=findPtr(*h,handleBlock);if(!off)return result(memWZErr);release(off); }
    *flags(h)=0;releaseMaster(h);return result(0);
}
int16_t MacHeap::emptyHandle(Handle h)
{
    if(!isHandle(h))return result(nilHandleErr);
    if(*flags(h)&0x80)return result(memPurErr);
    if(*h) { uint32_t off=findPtr(*h,handleBlock);if(!off)return result(memWZErr);release(off);*h=0; }
    return result(0);
}
int16_t MacHeap::reallocateHandle(Handle h,uint32_t bytes)
{
    if(!isHandle(h))return result(nilHandleErr);
    uint8_t saved=*flags(h);
    if(saved&0x80)return result(memPurErr);
    uint32_t old=*h ? findPtr(*h,handleBlock) : 0;
    if(*h && !old)return result(memWZErr);
    if(!physical(bytes))return result(memFullErr);
    // Keep the old pointer valid on failure. Moving other blocks is permitted.
    *flags(h)=saved|0x80;
    if(old) {
        compactUp();
        if(resizeInPlace(old,bytes)) { *flags(h)=1;return result(0); }
    }
    uint32_t off=allocate(bytes,handleBlock,(uint8_t*)h-arena_);
    if(!off) { *flags(h)=saved;return result(memFullErr); }
    *h=arena_+off+blockBytes;
    if(old)release(old);
    *flags(h)=1;return result(0);
}
uint32_t MacHeap::handleSize(Handle h)
{
    if(!isHandle(h) || !*h) { queryResult(nilHandleErr);return 0; }
    // The caller's live master slot was just checked. Validate its block and
    // back-pointer without looking up that identical master slot a second time.
    uint32_t off=owns(*h) ? findHandleBlock(*h,h) : 0;queryResult(off ? 0 : memWZErr);
    return off ? block(off).logical : 0;
}

int16_t MacHeap::setState(Handle h,uint8_t value)
{
    if(!isHandle(h))return result(nilHandleErr);
    *flags(h)=(value&0xe0)|1;return result(0);
}
MacHeap::Handle MacHeap::recoverHandle(uint8_t* ptr)
{
    if(owns(ptr))for(uint32_t off=headerBytes;off<end_;off+=block(off).span) {
        Block b=block(off);
        if(b.kind==handleBlock && ptr>=arena_+off+blockBytes && ptr<arena_+off+b.span) {
            queryResult(0);return (Handle)(arena_+b.owner);
        }
    }
    queryResult(nilHandleErr);return 0;
}
#include "HeapMoveHigh.inc"
int16_t MacHeap::setHandleSize(Handle h,uint32_t bytes)
{
    if(!isHandle(h) || !*h)return result(nilHandleErr);
    uint32_t off=findPtr(*h,handleBlock);if(!off)return result(memWZErr);
    if(resizeInPlace(off,bytes))return result(0);
    uint8_t saved=*flags(h);*flags(h)=saved&~0x40; // Allocation cannot purge its own source.
    if(saved&0x80) {
        compactUp();bool ok=resizeInPlace(off,bytes);*flags(h)=saved;
        return result(ok ? 0 : memFullErr);
    }
    // Relocate only this unlocked allocation when an existing hole fits.
    // Moving it high and compacting first can copy the surrounding megabytes
    // merely to grow a small region by a few bytes. Keep that recovery path
    // for fragmented/low-memory cases where no complete replacement fits.
    if(!findSpace(bytes)) {
        moveHigh(h);compact();off=findPtr(*h,handleBlock);
        if(resizeInPlace(off,bytes)) { *flags(h)=saved;return result(0); }
    }
    uint32_t oldSize=block(off).logical;
    uint32_t fresh=allocate(bytes,handleBlock,(uint8_t*)h-arena_);
    if(!fresh) { *flags(h)=saved;return result(memFullErr); }
    // allocate may compact; read the source from its unchanged master pointer.
    off=findPtr(*h,handleBlock);
    moveBytes(arena_+fresh+blockBytes,*h,oldSize<bytes ? oldSize : bytes);
    *h=arena_+fresh+blockBytes;release(off);*flags(h)=saved;return result(0);
}
bool MacHeap::check() const
{
    if(!arena_ || end_+trailerBytes!=bytes_)return false;
    uint32_t off=headerBytes,freeBytes=0;
    uint16_t masters=0;
    Handle previousFree=0;
    bool foundSearchStart=searchStart_==end_;
    while(off<end_) {
        const Block& b=block(off);
        if(b.span<minimumBlock || (b.span&7) || b.span>end_-off || b.kind>masterBlock)return false;
        if(b.kind!=freeBlock && b.logical>b.span-blockBytes)return false;
        if(b.kind==handleBlock) {
            Handle h=(Handle)(arena_+b.owner);
            if(!isHandle(h) || *h!=arena_+off+blockBytes
                || flags(h)!=arena_+b.stateOffset)return false;
            // The direct lookup must agree with the authoritative chain walk.
            if(findHandleBlock(*h)!=off || scanPtr(*h,handleBlock)!=off)return false;
        }
        if(off==searchStart_)foundSearchStart=true;
        if(b.kind==freeBlock) {if(off<searchStart_)return false;freeBytes+=b.span;}
        if(b.kind==masterBlock) {
            if(b.logical!=b.owner*(sizeof(uint8_t*)+1))return false;
            if(!masterBlocksOverflow_) {
                bool listed=false;
                for(uint16_t i=0;i<masterBlockCount_;++i)listed|=masterBlocks_[i]==off;
                if(!listed)return false;
            }
            ++masters;
            Handle handles=(Handle)(arena_+off+blockBytes);
            const uint8_t* states=(uint8_t*)(handles+b.owner);
            for(uint32_t i=0;i<b.owner;++i)if(!(states[i]&1)) {
                if(handles[i]!=(uint8_t*)previousFree)return false;
                previousFree=handles+i;
            }
        }
        off+=b.span;
    }
    if(!masterBlocksOverflow_ && masters!=masterBlockCount_)return false;
    return foundSearchStart && off==end_ && freeBytes==freeBytes_ && previousFree==freeMasters_;
}
