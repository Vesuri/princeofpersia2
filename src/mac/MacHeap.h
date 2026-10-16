#ifndef POP2_MAC_HEAP_H
#define POP2_MAC_HEAP_H
#include "PinnedSlotRanges.h"
// Fixed-width types supplied by the native ABI header or host <cstdint>.
// Arena and master-pointer blocks belong to the caller's zone. No host allocator.
class MacHeap {
public:
    typedef uint8_t** Handle;
    enum { noErr=0, memFullErr=-108, nilHandleErr=-109, memWZErr=-111,
           memPurErr=-112, memLockedErr=-117, paramErr=-50 };
    bool init(uint8_t* arena, uint32_t bytes, uint16_t masters=64);
    void reset();
    uint8_t* base() const { return arena_; }
    uint32_t capacity() const { return bytes_; }
    int16_t error() const { return error_; }
    bool owns(const void* address) const;
    uint8_t* newPtr(uint32_t bytes, bool clear=false);
    int16_t disposePtr(uint8_t* ptr);
    uint32_t ptrSize(uint8_t* ptr);
    int16_t setPtrSize(uint8_t* ptr, uint32_t bytes);
    Handle newHandle(uint32_t bytes, bool clear=false);
    Handle newEmptyHandle();
    int16_t disposeHandle(Handle handle);
    int16_t emptyHandle(Handle handle);
    int16_t reallocateHandle(Handle handle, uint32_t bytes);
    uint32_t handleSize(Handle handle);
    int16_t setHandleSize(Handle handle, uint32_t bytes);
    bool isHandle(Handle handle) const;
    bool isFreeHandleSlot(Handle handle) const;
    inline uint8_t state(Handle handle);
    inline int16_t lockHandle(Handle handle);
    int16_t setState(Handle handle, uint8_t state);
    Handle recoverHandle(uint8_t* ptr);
    int16_t moreMasters(uint16_t count=0);
    int16_t moveHigh(Handle handle);
    uint32_t compact();
    int16_t purge(uint32_t requested);
    uint32_t freeBytes() const;
    uint32_t largestBlock() const;
    bool check() const; // structural invariant, host/native diagnostic
private:
    struct Block { uint32_t span, logical, owner, kind, stateOffset, reserved; };
    enum { freeBlock=0, ptrBlock=1, handleBlock=2, masterBlock=3,
           headerBytes=64, trailerBytes=16, blockBytes=24, minimumBlock=32 };
    uint8_t* arena_ = 0;
    // Sum of free physical spans; moving/coalescing blocks preserves it.
    uint32_t bytes_ = 0, end_ = 0, freeBytes_ = 0;
    mutable uint32_t searchStart_ = headerBytes;
    uint16_t masters_ = 64;
    int16_t error_ = 0;
    Handle freeMasters_ = 0;
    // Master blocks are pinned and never released before reset, so these
    // offsets identify every master slot without walking the block chain.
    enum { maxMasterBlocks=32 };
    uint32_t masterBlocks_[maxMasterBlocks];
    uint16_t masterBlockCount_ = 0;
    bool masterBlocksOverflow_ = false;
    // Master blocks never move or shrink. Cache the last slot range, not its
    // mutable allocation flags or pointees; disposal/reuse stays observable.
    mutable uint32_t cachedMasterStart_ = 0, cachedMasterLength_ = 0;
    mutable PinnedSlotRanges masterRanges_;
    Block& block(uint32_t off) const { return *(Block*)(arena_+off); }
    static uint32_t physical(uint32_t logical);
    static void moveBytes(uint8_t* dst, const uint8_t* src, uint32_t bytes);
    static void reverseWords(uint8_t* first, uint8_t* last);
    __attribute__((always_inline)) uint8_t* flags(Handle handle) const {
        if(uint8_t* f=masterRanges_.cachedFlags(handle))return f;
        unsigned long base=(unsigned long)arena_;
        unsigned long relative=(unsigned long)handle-(base+cachedMasterStart_);
        if(relative<cachedMasterLength_ && relative%sizeof(uint8_t*)==0)
            return masterRanges_.rememberFlags(handle,arena_+cachedMasterStart_+cachedMasterLength_+relative/sizeof(uint8_t*));
        unsigned long pos=(unsigned long)handle-base;
        if(pos<bytes_)if(const auto* range=masterRanges_.find<sizeof(uint8_t*)>(pos)) {
            cachedMasterStart_=range->start;cachedMasterLength_=range->length;
            return masterRanges_.rememberFlags(handle,arena_+range->start+range->length+(pos-range->start)/sizeof(uint8_t*));
        }
        return flagsSlow(handle);
    }
    uint8_t* flagsSlow(Handle handle) const;
    uint8_t* masterFlags(uint32_t off, uint32_t pos) const;
    uint32_t findHandleBlock(const uint8_t* ptr, Handle validated=0) const;
    uint32_t scanPtr(const uint8_t* ptr, uint32_t kind) const;
    uint32_t findPtr(const uint8_t* ptr, uint32_t kind) const;
    uint32_t findSpace(uint32_t bytes) const;
    uint32_t allocate(uint32_t bytes, uint32_t kind, uint32_t owner=0);
    void split(uint32_t off, uint32_t span, uint32_t logical, uint32_t kind, uint32_t owner);
    void release(uint32_t off);
    void coalesce();
    bool movable(uint32_t off) const;
    void compactUp();
    bool resizeInPlace(uint32_t off, uint32_t bytes);
    void publish();
    void releaseMaster(Handle handle);
    // Queries update MemError without rebuilding unchanged zone metadata.
    int16_t queryResult(int16_t code) { error_=code; return code; }
    int16_t result(int16_t code) { error_=code; publish(); return code; }
};
#include "HeapLockHandle.inc"
#include "HeapReadHandleState.inc"
#endif
