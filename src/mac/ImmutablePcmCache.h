#pragma once
// Fixed-width types are supplied by the caller (including Amiga compat types).

// Bounded reuse of immutable PCM variants, following Alone in the Dark's
// prepared-song samples. Source ownership and immutability are caller contracts;
// mutable effects must not use pointer identity as a cache key. Entries in use
// cannot be evicted. Allocation failure preserves the uncached caller fallback.
// Memory supplies allocate(bytes)/free(pointer,bytes); Conversion supplies
// equal(plan,plan), bytes(plan), and convert(plan,destination).
template<class Plan,class Memory,class Conversion,unsigned Slots=16,
         uint32_t Budget=131072> class ImmutablePcmCache {
public:
    struct Entry {Plan plan={};uint8_t* data=nullptr;uint16_t users=0;};
private:
    Entry entries_[Slots]={};
    uint32_t retained_=0;
    void discard(Entry& e) {
        uint32_t bytes=Conversion::bytes(e.plan);
        Memory::free(e.data,bytes);retained_-=bytes;e=Entry();
    }
public:
    uint32_t retained() const {return retained_;}
    void clearUnused() {for(auto& e:entries_)if(e.data && !e.users)discard(e);}
    void release(Entry* entry) {if(entry && entry->users)--entry->users;}
    Entry* acquire(const Plan& plan) {
        uint32_t bytes=Conversion::bytes(plan);
        if(!bytes || bytes>Budget)return nullptr;
        for(auto& e:entries_)if(e.data && Conversion::equal(e.plan,plan)) {
            if(e.users==65535)return nullptr;
            ++e.users;return &e;
        }
        Entry* slot=nullptr;
        for(auto& e:entries_)if(!e.data){slot=&e;break;}
        if(!slot)for(auto& e:entries_)if(!e.users){slot=&e;break;}
        if(!slot)return nullptr;
        if(slot->data)discard(*slot);
        if(bytes>Budget-retained_)clearUnused();
        if(bytes>Budget-retained_)return nullptr;
        uint8_t* data=Memory::allocate(bytes);
        if(!data)return nullptr;
        Conversion::convert(plan,data);
        slot->plan=plan;slot->data=data;slot->users=1;retained_+=bytes;
        return slot;
    }
};
