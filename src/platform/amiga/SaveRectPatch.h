#pragma once
#ifndef __mc68000__
#include <stdint.h>
#endif
#include "../../mac/BigEndianMemory.h"
namespace SaveRectPatch {
// Original input is SHA-256 pinned by installation and the host fixture. This
// small load-time fingerprint guards the exact instruction extent we replace;
// it is not a whole-file release startup verification pass.
inline bool install(uint8_t* code,uint32_t size,uint32_t entry) {
    const uint32_t begin=0x53a4,end=0x53c6;
    if(size<end || !entry || (entry&1))return false;
    uint32_t fingerprint=2166136261UL;
    for(uint32_t i=begin;i<end;++i)fingerprint=(fingerprint^code[i])*16777619UL;
    if(fingerprint!=0xbdc7348bUL)return false;
    BigEndianMemory::write16(code+begin,0x4eb9); // JSR native row
    BigEndianMemory::write32(code+begin+2,entry);
    BigEndianMemory::write16(code+begin+6,0x6000); // BRA.W original row advance
    BigEndianMemory::write16(code+begin+8,end-(begin+8));
    return true;
}
}
