#ifndef AMIGAXDEV_VBL_PASSES_H
#define AMIGAXDEV_VBL_PASSES_H
#ifndef __mc68000__
#include <stdint.h>
#endif
// Queued virtual retrace passes from Vette! and Alone in the Dark.
// Safe-point callers own task records, countdown semantics and callbacks.
// Age all entries on begin(), then drain that fixed pass in queue order.
// New entries join the following pass; removal precedes array compaction.
struct VBLPasses {
    uint16_t index=0,limit=0;
    uint32_t pending=0,last=0,tick=0;
    bool active=false;
    void reset(uint32_t now) {
        index=limit=0;pending=0;active=false;last=tick=now;
    }
    void observe(uint32_t now) {
        uint32_t elapsed=now-last;
        if(elapsed) {
            last=now;
            uint32_t room=0xffffffffu-pending;
            pending+=elapsed<room ? elapsed : room;
        }
    }
    bool begin(uint16_t count) {
        if(active || !pending)return false;
        --pending;++tick;index=0;limit=count;active=true;return true;
    }
    void removed(uint16_t position) {
        if(active) {
            if(index>position)--index;
            if(limit>position)--limit;
            if(index>=limit)active=false;
        }
    }
    bool needed(uint32_t now) const {return last!=now || pending || active;}
};
#endif
