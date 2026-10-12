#ifndef POP2_AGA_PALETTE_H
#define POP2_AGA_PALETTE_H
#ifndef POP2_PLATFORM_AMIGA
#include <stdint.h>
#endif

// The full 24-bit AGA palette is written through eight banks of 32 registers.
// Input is already display RGB24; Mac colour transfer belongs upstream.
namespace AgaPalette {
static const uint16_t control=0x0c60; // low-resolution sprites, black border, bank zero, LOCT clear
static const uint16_t moves=8*(1+32+1+32)+1;
inline uint32_t move(uint16_t reg,uint16_t value) {return uint32_t(reg)<<16|value;}
inline void build(uint32_t* copper,const uint32_t* rgb,uint8_t playfieldXor=0) {
    uint16_t out=0;
    for(uint16_t bank=0;bank<8;++bank) {
        copper[out++]=move(0x106,uint16_t(control|(bank<<13)));
        for(uint16_t pen=0;pen<32;++pen) {
            uint32_t color=rgb[(bank*32+pen)^playfieldXor];
            copper[out++]=move(uint16_t(0x180+pen*2),
                uint16_t(((color>>12)&0xf00)|((color>>8)&0xf0)|((color>>4)&0xf)));
        }
        copper[out++]=move(0x106,uint16_t(control|(bank<<13)|0x200));
        for(uint16_t pen=0;pen<32;++pen) {
            uint32_t color=rgb[(bank*32+pen)^playfieldXor];
            copper[out++]=move(uint16_t(0x180+pen*2),
                uint16_t(((color>>8)&0xf00)|((color>>4)&0xf0)|(color&0xf)));
        }
    }
    copper[out]=move(0x106,control);
}
}
#endif
