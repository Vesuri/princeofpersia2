// Shared in-place resize for arena/master-block MacHeap adapters.
// Include at file scope; the adapter owns errors, publication and recovery.
bool MacHeap::resizeInPlace(uint32_t off,uint32_t bytes)
{
    uint32_t needed=physical(bytes);if(!needed)return false;
    Block saved=block(off);
    if(needed>saved.span) {
        uint32_t next=off+saved.span;
        if(next>=end_ || block(next).kind!=freeBlock || saved.span+block(next).span<needed)return false;
        freeBytes_-=block(next).span;
        block(off).span+=block(next).span;
    }
    split(off,needed,bytes,saved.kind,saved.owner);
    // The zone is already coalesced. Only the tail just split from this
    // allocation can have gained a free neighbour; no preceding block or
    // unrelated allocation changed. Preserve the exact old physical layout.
    uint32_t tail=off+block(off).span;
    if(tail<end_ && block(tail).kind==freeBlock) {
        uint32_t next=tail+block(tail).span;
        if(next<end_ && block(next).kind==freeBlock)block(tail).span+=block(next).span;
    }
    return true;
}
