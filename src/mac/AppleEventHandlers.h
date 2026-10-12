#ifndef POP2_APPLE_EVENT_HANDLERS_H
#define POP2_APPLE_EVENT_HANDLERS_H
#ifndef POP2_PLATFORM_AMIGA
#include <stdint.h>
#endif
// Application-owned registrations only. No callback invocation or event delivery.
class AppleEventHandlers {
public:
    struct Entry { uint32_t eventClass, eventID, handler, refCon; };
    static const uint16_t capacity=32;
    Entry entries[capacity];
    uint16_t count=0;
    void reset() { count=0; }
    bool install(uint32_t eventClass,uint32_t eventID,uint32_t handler,
                 uint32_t refCon,uint8_t system,int16_t& error) {
        if(system || eventClass==0x2a2a2a2aUL || eventID==0x2a2a2a2aUL)return false;
        if(!handler || (handler&1)) { error=-50;return true; }
        uint16_t i=0;
        while(i<count && (entries[i].eventClass!=eventClass || entries[i].eventID!=eventID))++i;
        if(i==capacity)return false; // Capacity is an explicit unsupported stop, not guessed memFullErr.
        entries[i].eventClass=eventClass;entries[i].eventID=eventID;
        entries[i].handler=handler;entries[i].refCon=refCon;
        if(i==count)++count;
        error=0;return true;
    }
    bool lookup(uint32_t eventClass,uint32_t eventID,uint8_t system,
                uint32_t& handler,uint32_t& refCon,int16_t& error) const {
        if(system>1 || eventClass==0x2a2a2a2aUL || eventID==0x2a2a2a2aUL)return false;
        // The port installs no system handlers. The application table is separate.
        if(!system)for(uint16_t i=0;i<count;++i) {
            const Entry& e=entries[i];
            if(e.eventClass==eventClass && e.eventID==eventID) {
                handler=e.handler;refCon=e.refCon;error=0;return true;
            }
        }
        error=-1717;return true; // Missing lookups leave both output values untouched.
    }
};
#endif
