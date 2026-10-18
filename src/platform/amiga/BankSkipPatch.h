#pragma once
#ifndef __mc68000__
#include <stdint.h>
#endif
#include "../../mac/BigEndianMemory.h"
namespace BankSkipPatch {
// Guard the complete original inner block, including decoding and MOVEM restore.
// Positive clipped widths constrain the transparent run's DBRA count to 0..127.
// Scratch D5/CCR differ inside the block; the original continuation restores them.
inline bool install(uint8_t* code,uint32_t size) {
    const uint32_t begin=0x90,end=0x140;
    if(size<end)return false;
    uint32_t fingerprint=2166136261UL;
    for(uint32_t i=begin;i<end;++i)fingerprint=(fingerprint^code[i])*16777619UL;
    if(fingerprint!=0xcf6da9cbUL)return false;
    BigEndianMemory::write16(code+0x10e,0x94c5); // SUBA.W D5,A2
    BigEndianMemory::write16(code+0x110,0x538a); // SUBQ.L #1,A2
    BigEndianMemory::write16(code+0x112,0x7aff); // MOVEQ #-1,D5
    BigEndianMemory::write16(code+0x114,0x600c); // BRA.B original run advance
    return true;
}
}
