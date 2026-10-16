// Shared MoveHHi implementation for the arena/master-block MacHeap adapters.
// Include at file scope after MacHeap.h and the adapter's block/copy helpers.
// Keeps local errors, state publication and allocation policy in the adapter.
int16_t MacHeap::moveHigh(Handle h)
{
    if(!isHandle(h) || !*h)return result(nilHandleErr);
    if(*flags(h)&0x80)return result(memLockedErr);
    uint32_t off=findPtr(*h,handleBlock);if(!off)return result(memWZErr);
    uint32_t span=block(off).span,limit=off+span,tail=off;
    while(limit<end_ && (block(limit).kind==freeBlock || movable(limit))) {
        tail=limit;limit+=block(limit).span;
    }
    // System 7.5.5 moves only the target when the free tail below the next
    // immovable block fits. Interior holes do not qualify: the reference
    // compacts neighbours to put the target at the very top of this interval.
    if(tail!=off && block(tail).kind==freeBlock && block(tail).span>=span) {
        Block saved=block(off);
        uint32_t remaining=block(tail).span-span;
        uint32_t destination=remaining>=minimumBlock ? limit-span : tail;
        if(remaining<minimumBlock)saved.span=block(tail).span;
        moveBytes(arena_+destination+blockBytes,arena_+off+blockBytes,saved.logical);
        if(remaining>=minimumBlock)block(tail).span=remaining;
        block(destination)=saved;freeBytes_-=saved.span;
        *h=arena_+destination+blockBytes;
        release(off);return result(0);
    }
    const uint32_t intervalStart=off;
    uint32_t next=off+span;
    if(next<limit && block(next).kind==freeBlock) {
        // Swap past unused space without reading/copying its payload. Continue
        // the same rotation through any live blocks beyond the hole, retaining
        // their order and the target's final address before the next barrier.
        uint32_t gap=block(next).span;
        moveBytes(arena_+off+gap,arena_+off,span);
        block(off)={gap,0,0,freeBlock,0,0};
        off+=gap;
    }
    // Nothing follows the target after a free-gap swap: only publish its new
    // master pointer; avoid reversing a potentially large sample twice.
    if(off+span==limit) { *h=arena_+off+blockBytes;coalesce();return result(0); }
    // Rotate blocks without allocating scratch memory; immovable barriers remain fixed.
    reverseWords(arena_+off,arena_+off+span);
    reverseWords(arena_+off+span,arena_+limit);
    reverseWords(arena_+off,arena_+limit);
    for(uint32_t at=off;at<limit;at+=block(at).span)
        if(block(at).kind==handleBlock)*(Handle)(arena_+block(at).owner)=arena_+at+blockBytes;
    // The original compacts the neighbours below the high target, rather than
    // leaving interior holes at the low end of the interval after rotation.
    uint32_t dst=intervalStart;
    for(uint32_t src=intervalStart;src<limit-span;) {
        Block saved=block(src);uint32_t next=src+saved.span;
        if(saved.kind!=freeBlock) {
            if(dst!=src)moveBytes(arena_+dst,arena_+src,saved.span);
            *(Handle)(arena_+saved.owner)=arena_+dst+blockBytes;
            dst+=saved.span;
        }
        src=next;
    }
    if(dst<limit-span)block(dst)={limit-span-dst,0,0,freeBlock,0,0};
    coalesce();return result(0);
}
