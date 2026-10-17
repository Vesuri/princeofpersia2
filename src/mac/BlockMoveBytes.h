#pragma once
#ifndef __mc68000__
#include <stdint.h>
#endif

// Byte-exact 68020 memmove. Longword loads precede stores; traversal direction
// preserves overlap even less than a longword apart. Never read outside the
// requested interval. unsigned long holds Amiga LP32 and host LP64 pointers.
namespace BlockMoveBytes {
inline void word(const uint8_t* source, uint8_t* destination) {
#ifdef __mc68000__
    struct __attribute__((packed,may_alias)) Long { uint32_t value; };
    // The 68020 permits unaligned destinations. Explicit MOVE.L prevents
    // packed accesses from becoming four separate byte loads and stores.
    __asm__ volatile("move.l %1,%0"
        : "=m"(*(Long*)destination) : "m"(*(const Long*)source) : "cc");
#else
    uint8_t a=source[0],b=source[1],c=source[2],d=source[3];
    destination[0]=a;destination[1]=b;destination[2]=c;destination[3]=d;
#endif
}
inline void bulk(const uint8_t*& source,uint8_t*& destination,
                 uint32_t chunks,bool backwards) {
#ifdef __mc68000__
    // chunks is positive. Keep the whole loop in assembly so the compiler
    // cannot copy/advance a second set of pointers around each unrolled block.
    if (backwards) {
        __asm__ volatile(
            "1: .rept 32\n\tmove.l -(%0),-(%1)\n\t.endr\n\t"
            "subq.l #1,%2\n\tbne.s 1b"
            : "+a"(source), "+a"(destination), "+d"(chunks) : : "cc","memory");
    } else {
        __asm__ volatile(
            "1: .rept 32\n\tmove.l (%0)+,(%1)+\n\t.endr\n\t"
            "subq.l #1,%2\n\tbne.s 1b"
            : "+a"(source), "+a"(destination), "+d"(chunks) : : "cc","memory");
    }
#else
    while (chunks--) for(unsigned i=0;i<32;++i) {
        if(backwards) {source-=4;destination-=4;word(source,destination);}
        else {word(source,destination);source+=4;destination+=4;}
    }
#endif
}
inline void move(const uint8_t* source, uint8_t* destination, uint32_t count) {
    if (!count || source==destination) return;
    unsigned long src=(unsigned long)source,dst=(unsigned long)destination;
    if (dst>src && dst-src<count) {
        source+=count;destination+=count;
        while (count && ((unsigned long)source&3)) {
            *--destination=*--source;--count;
        }
        uint32_t chunks=count>>7;
        if(chunks) {bulk(source,destination,chunks,true);count&=127;}
        while (count>=4) {
            source-=4;destination-=4;word(source,destination);count-=4;
        }
        while (count) { *--destination=*--source;--count; }
    } else {
        while (count && ((unsigned long)source&3)) {
            *destination++=*source++;--count;
        }
        uint32_t chunks=count>>7;
        if(chunks) {bulk(source,destination,chunks,false);count&=127;}
        while (count>=4) {
            word(source,destination);source+=4;destination+=4;count-=4;
        }
        while (count) { *destination++=*source++;--count; }
    }
}
}
