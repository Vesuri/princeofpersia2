#pragma once
#ifndef POP2_PLATFORM_AMIGA
#include <cstdint>
#endif
// Measured System 7.5.5 / IIci answers. Unknown selectors are unsupported.
namespace MacIdentity {
inline bool gestalt(uint32_t selector,uint32_t& response,int16_t& error) {
    response=0;error=0;
    switch(selector) {
    case 0x766d2020: break;                 // vm: disabled
    case 0x70726f63: response=4;break;       // proc: 68030 reference
    case 0x73797376: response=0x0755;break;  // sysv
    case 0x6d616368: response=11;break;      // mach: IIci
    case 0x6d6d7520: response=3;break;       // mmu
    case 0x61646472: response=4;break;       // addr: reference attributes
    case 0x612f7578: error=-5550;break;      // a/ux: unknown answer
    default:return false;
    }
    return true;
}
inline void word(uint8_t* p,uint16_t v) { p[0]=v>>8;p[1]=v; }
inline bool sysEnvirons(uint16_t requested,uint8_t* p) {
    if(requested!=1 || !p)return false;
    word(p,1);word(p+2,9);                  // record version, IIci environment ID
    word(p+4,0x0755);word(p+6,4);           // System 7.5.5, 68030
    p[8]=1;p[9]=1;word(p+10,5);            // FPU, Color QuickDraw, keyboard
    word(p+12,0x003a);word(p+14,0x8053);    // measured driver version, system vRef
    return true;
}
}
