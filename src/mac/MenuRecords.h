#ifndef POP2_MENU_RECORDS_H
#define POP2_MENU_RECORDS_H
#ifndef POP2_PLATFORM_AMIGA
#include <stdint.h>
#endif
// Packed classic MenuInfo records; excludes menu drawing and MDEF execution.
class MenuRecords {
public:
    struct Item { uint32_t offset=0; uint16_t length=0; };
    static bool scan(const uint8_t* data,uint32_t size,uint16_t requested,
                     Item& found,uint16_t& count) {
        found.offset=0;found.length=0;count=0;
        if(!data || size<16 || uint32_t(15)+data[14]>=size)return false;
        uint32_t pos=15+data[14];
        while(data[pos]) {
            uint32_t length=data[pos];
            if(length+5>=size-pos || count==32767)return false;
            ++count;
            if(count==requested) { found.offset=pos;found.length=length; }
            pos+=length+5;
        }
        return !requested || found.offset;
    }
    static void copyByte(uint8_t* dest,const uint8_t* source) {
        // Avoid GCC 15's shared-base postincrement memory-to-memory byte form.
        volatile uint8_t value=*source;*dest=value;
    }
    static bool get(const uint8_t* data,uint32_t size,uint16_t number,uint8_t* text) {
        Item item;uint16_t count;
        if(!number || !text || !scan(data,size,number,item,count))return false;
        for(uint16_t i=0;i<=item.length;++i)copyByte(text+i,data+item.offset+i);
        return true;
    }
    static bool replacement(const uint8_t* data,uint32_t size,uint16_t number,
                            uint16_t length,Item& item,uint32_t& newSize) {
        uint16_t count;
        // Empty labels and nonexistent item behavior remain explicit stops.
        if(!number || !length || length>255 || !scan(data,size,number,item,count))return false;
        if(size-item.length>0x7fffffffUL-length)return false;
        newSize=size-item.length+length;return true;
    }
    // Caller provides a stable Pascal string, checked plan and enough storage.
    // Shrink the handle only after moving the tail; grow it before this call.
    static void replace(uint8_t* data,uint32_t oldSize,const Item& item,const uint8_t* text) {
        uint32_t oldTail=item.offset+1+item.length,newTail=item.offset+1+text[0];
        if(newTail>oldTail) {
            for(uint32_t n=oldSize-oldTail;n;--n)copyByte(data+newTail+n-1,data+oldTail+n-1);
        } else {
            for(uint32_t n=0;n<oldSize-oldTail;++n)copyByte(data+newTail+n,data+oldTail+n);
        }
        for(uint16_t i=0;i<=text[0];++i)copyByte(data+item.offset+i,text+i);
        // SetMenuItemText invalidates cached menu dimensions on the reference.
        data[2]=255;data[3]=255;data[4]=255;data[5]=255;
    }
};
#endif
