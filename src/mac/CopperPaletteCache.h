#ifndef AMIGAXDEV_COPPER_PALETTE_CACHE_H
#define AMIGAXDEV_COPPER_PALETTE_CACHE_H
#ifndef __mc68000__
#include <stdint.h>
#endif
// Inactive-list palette reuse, following Alone in the Dark's per-list palette
// generations. Content comparison also handles callers without a reliable seed;
// invalidation bits avoid generation-wrap aliases. Caller never edits an active
// list, and marks a list only after writing its complete palette.
namespace CopperPaletteCache {
template<unsigned Colors,unsigned Lists> class State {
    static_assert(Lists>0 && Lists<=32,"palette list mask");
    uint32_t colors_[Colors]={};
    uint32_t key_=0,ready_=0;
    bool valid_=false;
public:
    void reset() {valid_=false;ready_=0;}
    bool update(const uint32_t* colors,uint32_t key=0) {
        bool changed=!valid_ || key!=key_;
        for(unsigned i=0;!changed && i<Colors;++i)changed=colors_[i]!=colors[i];
        if(changed) {
            for(unsigned i=0;i<Colors;++i)colors_[i]=colors[i];
            key_=key;ready_=0;valid_=true;
        }
        return changed;
    }
    bool needs(unsigned list) const {return list<Lists && valid_ && !(ready_&(uint32_t(1)<<list));}
    void mark(unsigned list) {if(list<Lists && valid_)ready_|=uint32_t(1)<<list;}
    const uint32_t* colors() const {return colors_;}
};
}
#endif
