#pragma once
// Fixed-width types come from the caller. Cache only immutable master-block
// ranges, never allocation flags or pointees. Clear whenever the arena resets.
class PinnedSlotRanges {
public:
    struct Range {uint32_t start=0,length=0;};
private:
    Range ranges_[16]={};
    struct Slot {const void* address=nullptr;uint8_t* flags=nullptr;};
    Slot slots_[32]={};
    static unsigned slotIndex(const void* slot){return ((unsigned long)slot>>2)&31;}
public:
    void clear(){for(auto& r:ranges_)r=Range();for(auto& s:slots_)s=Slot();}
    // The caller supplies this mapping only after validating a pinned slot.
    // Cache its address, never its mutable flags or pointee. Reset clears both
    // kinds of mapping before any old master storage may be reused.
    uint8_t* cachedFlags(const void* slot) const {
        const auto& s=slots_[slotIndex(slot)];
        return s.address==slot?s.flags:nullptr;
    }
    uint8_t* rememberFlags(const void* slot,uint8_t* flags) {
        slots_[slotIndex(slot)]={slot,flags};return flags;
    }
    void remember(uint32_t position,uint32_t start,uint32_t length){
        ranges_[(position>>8)&15]={start,length};
    }
    template<unsigned SlotBytes> const Range* find(uint32_t position) const {
        const auto& r=ranges_[(position>>8)&15];
        uint32_t offset=position-r.start;
        return offset<r.length && !(offset%SlotBytes) ? &r : nullptr;
    }
};
