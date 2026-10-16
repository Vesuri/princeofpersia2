// Shared first-fit search for the arena/master-block MacHeap adapters.
// searchStart_ is a conservative physical block boundary: no free block may
// precede it. Cache only the occupied prefix, never a size-dependent fit.
// Adapter contract: reset it to headerBytes at init/reset and after compaction;
// rewind it to off on split; coalesce must publish its first free block (or
// end_). MoveHHi finishes through release/coalesce. No address escapes the heap.
uint32_t MacHeap::findSpace(uint32_t bytes) const
{
    uint32_t needed=physical(bytes);if(!needed)return 0;
    uint32_t first=end_;
    for(uint32_t off=searchStart_;off<end_;off+=block(off).span) {
        if(block(off).kind!=freeBlock)continue;
        if(first==end_)first=off;
        if(block(off).span>=needed) { searchStart_=first;return off; }
    }
    searchStart_=first;
    return 0;
}
