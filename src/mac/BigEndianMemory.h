#pragma once
#ifndef __mc68000__
#include <stdint.h>
#endif
// Existing Alone in the Dark 68020 accesses, shared with the other Mac ports.
// The target supports unaligned words/longs; packed alias types describe them
// without alignment or strict-aliasing assumptions. Host checks use byte order.
namespace BigEndianMemory {
#ifdef __mc68000__
struct __attribute__((packed, may_alias)) Word { uint16_t value; };
struct __attribute__((packed, may_alias)) Long { uint32_t value; };
#endif
#ifdef __mc68000__
#define AMIGAXDEV_MEMORY_READ16(p,v) __asm__("move.w %1,%0" : "=d"(v) : "m"(*(const BigEndianMemory::Word*)(p)) : "cc")
#define AMIGAXDEV_MEMORY_READ32(p,v) __asm__("move.l %1,%0" : "=d"(v) : "m"(*(const BigEndianMemory::Long*)(p)) : "cc")
#define AMIGAXDEV_MEMORY_WRITE16(p,v) __asm__ volatile("move.w %1,%0" : "=m"(*(BigEndianMemory::Word*)(p)) : "d"(v) : "cc")
#define AMIGAXDEV_MEMORY_WRITE32(p,v) __asm__ volatile("move.l %1,%0" : "=m"(*(BigEndianMemory::Long*)(p)) : "d"(v) : "cc")
#endif
#ifndef AMIGAXDEV_MEMORY_PRIMITIVES_ONLY
inline uint16_t read16(const uint8_t* p) {
#ifdef __mc68000__
    uint16_t value;
    AMIGAXDEV_MEMORY_READ16(p,value);
    return value;
#else
    return uint16_t(p[0])<<8|p[1];
#endif
}
inline uint32_t read32(const uint8_t* p) {
#ifdef __mc68000__
    uint32_t value;
    AMIGAXDEV_MEMORY_READ32(p,value);
    return value;
#else
    return uint32_t(read16(p))<<16|read16(p+2);
#endif
}
inline void write16(uint8_t* p,uint16_t value) {
#ifdef __mc68000__
    AMIGAXDEV_MEMORY_WRITE16(p,value);
#else
    p[0]=uint8_t(value>>8);p[1]=uint8_t(value);
#endif
}
inline void write32(uint8_t* p,uint32_t value) {
#ifdef __mc68000__
    AMIGAXDEV_MEMORY_WRITE32(p,value);
#else
    write16(p,uint16_t(value>>16));write16(p+2,uint16_t(value));
#endif
}
#endif
}
