#pragma once
#include "RectBounds.h"
// The caller has checked all three eight-byte spans. Read every coordinate
// before writing: output may overlap either input, including at odd addresses.
namespace RectUnion68k {
inline void unite(uint8_t* out,const uint8_t* a,const uint8_t* b) {
#ifdef __mc68000__
    uint32_t top,left,bottom,right;
    __asm__ volatile(
        "move.w (%[a]),%[t]\n\tcmp.w (%[b]),%[t]\n\tble.s 1f\n\tmove.w (%[b]),%[t]\n1:\n\t"
        "move.w 2(%[a]),%[l]\n\tcmp.w 2(%[b]),%[l]\n\tble.s 2f\n\tmove.w 2(%[b]),%[l]\n2:\n\t"
        "move.w 4(%[a]),%[d]\n\tcmp.w 4(%[b]),%[d]\n\tbge.s 3f\n\tmove.w 4(%[b]),%[d]\n3:\n\t"
        "move.w 6(%[a]),%[r]\n\tcmp.w 6(%[b]),%[r]\n\tbge.s 4f\n\tmove.w 6(%[b]),%[r]\n4:\n\t"
        "move.w %[t],(%[o])\n\tmove.w %[l],2(%[o])\n\tmove.w %[d],4(%[o])\n\tmove.w %[r],6(%[o])"
        : [t] "=&d"(top),[l] "=&d"(left),[d] "=&d"(bottom),[r] "=&d"(right)
        : [a] "a"(a),[b] "a"(b),[o] "a"(out) : "cc","memory");
#else
    RectBounds::unite(out,a,b);
#endif
}
}
