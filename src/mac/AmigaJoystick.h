#ifndef POP2_AMIGA_JOYSTICK_H
#define POP2_AMIGA_JOYSTICK_H
#ifndef POP2_PLATFORM_AMIGA
#include <stdint.h>
#endif
// Port 1 controller as held Amiga raw keys, translated like the keyboard.
// Owner mapping: directions are the cursor keys, the second button is Up
// (jump), and fire is Space in cinematics, Ctrl with an enemy and Shift
// otherwise. JOY1DAT is crosswise quadrature (forward = b8^b9,
// left = b9, back = b0^b1, right = b1), fire is CIA-A PRA bit 7 and the
// second button POTINP bit 14, all active as read here.
static const uint8_t joystickRaw[4]={0x4c,0x4d,0x4e,0x4f};
static const uint8_t joystickFire=0x10;
enum:uint8_t{fireSpace=0x40,fireShift=0x60,fireCtrl=0x63};
inline uint8_t joystickHeld(uint16_t joy,uint8_t ciaPra,uint16_t potinp)
{
    const bool up=((joy>>9)^(joy>>8))&1,down=((joy>>1)^joy)&1;
    const bool left=(joy>>9)&1,right=(joy>>1)&1;
    const bool fire=!(ciaPra&0x80),second=!(potinp&0x4000);
    return uint8_t((up||second?1:0)|(down?2:0)|(right?4:0)|(left?8:0)|(fire?joystickFire:0));
}
// DERIVED: original ReadKeyboard (CODE 2+$5614) prefers Shift over Ctrl, so
// fire presses one key, chosen from original state when it is pressed.
// The mode word A5-$5000 is 1 in a cinematic (CODE 2+$02A4). MEASURED: it is
// -1 in the first room of play. An enemy is a living character in the kid's room, as
// CODE 6+$36F2 selects them: records of 62 bytes from A5-$5208, counted at
// context+$20E6+192*room (room A5-$50BC), alive while word +$1C is negative,
// except a type 7/8 whose room record (room A5-$4FC2) has word +$1C of 2.
// Memory supplies a5(offset) and at(address) big-endian words, and context().
template<class Memory> uint8_t joystickFireKey(const Memory& m)
{
    if(m.a5(-0x5000)==1)return fireSpace;
    const uint32_t context=m.context();
    if(!context)return fireShift;
    auto count=[&](uint16_t room)->uint16_t{return room && room<=32 ? m.at(context+0x20e6+192u*room) : 0;};
    const uint16_t opponents=count(m.a5(-0x50bc));
    for(uint16_t i=0;i<opponents && i<5;++i) {
        const int32_t record=-0x5208+62*i;
        if(int16_t(m.a5(record+0x1c))>=0)continue;
        const uint16_t type=m.a5(record+8),room=m.a5(-0x4fc2);
        if((type==7 || type==8) && i<count(room) && m.at(context+0x20e6+192u*room+2+38u*i+0x1c)==2)continue;
        return fireCtrl;
    }
    return fireShift;
}
#endif
