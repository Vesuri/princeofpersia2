#pragma once
#ifndef __mc68000__
#include <stdint.h>
#endif
// Callers supply disjoint [base, byte-size] ranges. Every hit rereads the
// current range, so unloading, resizing or replacing it invalidates stale hits.
// Offsets must leave room for a complete 68k instruction word.
class CodeRangeCache {
    uint32_t recent_[64]={}; // index+1; zero means no cached range.
    static bool contains(const uint32_t* range,uint32_t pc) {
        return range[1]>=2 && pc>=range[0] && pc-range[0]<=range[1]-2;
    }
public:
    bool find(const uint32_t (*ranges)[2],uint32_t count,uint32_t pc,
              uint32_t& index,uint32_t& offset) {
        const unsigned slot=(pc>>12)&63;
        uint32_t cached=recent_[slot];
        if(cached && cached<=count && contains(ranges[cached-1],pc)) {
            index=cached-1;offset=pc-ranges[index][0];return true;
        }
        for(uint32_t i=0;i<count;++i)if(contains(ranges[i],pc)) {
            recent_[slot]=i+1;
            index=i;offset=pc-ranges[i][0];return true;
        }
        return false;
    }
};
