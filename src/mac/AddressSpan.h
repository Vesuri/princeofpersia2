#pragma once
#ifndef __mc68000__
#include <stdint.h>
#endif
// Integer address windows whose exclusive end fits in the address space.
// Validate each owned allocation once before calling contains(). No cached
// allocation flags or pointees are involved.
namespace AddressSpan {
inline bool valid(uint32_t base,uint32_t bytes){return bytes<=uint32_t(-1)-base;}
inline bool contains(uint32_t base,uint32_t bytes,uint32_t at,uint32_t count){
    // With valid(base,bytes), an address below base wraps to a difference
    // greater than bytes, so this also performs the lower-bound check.
    return count<=bytes && uint32_t(at-base)<=bytes-count;
}
}
