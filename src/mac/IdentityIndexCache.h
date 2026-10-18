#pragma once
#ifndef __mc68000__
#include <stdint.h>
#endif
// Bounded hints for records with unique live identities. The caller's predicate
// must validate both identity and current liveness. Hits reread the record, so
// removal, slot reuse, count changes and collisions need no invalidation pass.
// This does not replace a resource-chain or other namespace lookup.
template<unsigned Slots=128>
class IdentityIndexCache {
    static_assert(Slots && !(Slots&(Slots-1)),"power-of-two hint slots");
    uint32_t hints_[Slots]={}; // index+1; zero is an empty hint
public:
    template<class Record,class Matches>
    bool find(const Record* records,uint32_t count,uint32_t identity,
              Matches matches,uint32_t& index) {
        uint32_t& hint=hints_[identity&(Slots-1)];
        if(hint && hint<=count && matches(records[hint-1],identity)) {
            index=hint-1;return true;
        }
        for(uint32_t i=0;i<count;++i)if(matches(records[i],identity)) {
            hint=i+1;index=i;return true;
        }
        return false;
    }
};
