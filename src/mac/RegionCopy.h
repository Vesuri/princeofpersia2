#ifndef POP2_REGION_COPY_H
#define POP2_REGION_COPY_H
#include "MacHeap.h"
namespace RegionCopy {
// Copy the encoded region, not its bounding box. Lock the source while the
// destination grows: compaction/purging must not invalidate the input.
inline int16_t copy(MacHeap& sourceZone, MacHeap::Handle source,
                    MacHeap& destinationZone, MacHeap::Handle destination,
                    uint16_t& size) {
    size=0;
    if(!sourceZone.isHandle(source) || !destinationZone.isHandle(destination))
        return MacHeap::memWZErr;
    if(!*source || !*destination)return MacHeap::nilHandleErr;
    if(sourceZone.handleSize(source)<10)return MacHeap::paramErr;
    uint16_t bytes=uint16_t(uint16_t((*source)[0])<<8|(*source)[1]);
    if(bytes<10 || bytes>32766 || (bytes&1) || bytes>sourceZone.handleSize(source))
        return MacHeap::paramErr;
    if(source!=destination) {
        uint8_t state=sourceZone.state(source);
        sourceZone.setState(source,state|0x80);
        int16_t error=destinationZone.setHandleSize(destination,bytes);
        if(!error)for(uint16_t i=0;i<bytes;++i) {
            volatile uint8_t value=(*source)[i];(*destination)[i]=value;
        }
        sourceZone.setState(source,state);
        if(error)return error;
    }
    size=bytes;return MacHeap::noErr;
}
}
#endif
