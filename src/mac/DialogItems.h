#ifndef POP2_DIALOG_ITEMS_H
#define POP2_DIALOG_ITEMS_H
#ifndef POP2_PLATFORM_AMIGA
#include <stdint.h>
#endif
// Validate the complete packed DITL before any item is materialized.
class DialogItems {
public:
    struct Item { uint32_t offset; uint8_t type,length; };
    static bool scan(const uint8_t* bytes,uint32_t size,Item* items,uint16_t capacity,uint16_t& count) {
        count=0;if(!bytes || size<2 || !items)return false;
        uint32_t total=(uint32_t(bytes[0])<<8 | bytes[1])+1;
        if(total>capacity)return false;
        uint32_t pos=2;
        for(uint32_t i=0;i<total;++i) {
            if(pos>size || size-pos<14)return false;
            uint8_t length=bytes[pos+13];
            uint32_t next=pos+14+length;next+=(next&1);
            if(next>size)return false;
            items[i].offset=pos;items[i].type=bytes[pos+12];items[i].length=length;
            pos=next;
        }
        if(pos!=size)return false;
        count=(uint16_t)total;return true;
    }
};
#endif
