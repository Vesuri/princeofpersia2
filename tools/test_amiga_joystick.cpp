#include "mac/AmigaJoystick.h"
#include "mac/AmigaKeyMap.h"
#include <cassert>
#include <cstdio>
#include <map>
// Synthetic original state: A5 words, a context block and its room tables.
struct Memory {
 std::map<int32_t,uint16_t> world;std::map<uint32_t,uint16_t> block;uint32_t base=0x100000;
 uint16_t a5(int32_t offset)const{auto i=world.find(offset);return i==world.end()?0:i->second;}
 uint32_t context()const{return base;}
 uint16_t at(uint32_t address)const{auto i=block.find(address);return i==block.end()?0:i->second;}
 void opponent(unsigned slot,uint16_t type,bool alive){world[-0x5208+62*slot+8]=type;world[-0x5208+62*slot+0x1c]=alive?0xffff:3;}
 uint32_t room(uint16_t room){return base+0x20e6+192u*room;}
};
int main(){
 const uint8_t idleCia=0xff;const uint16_t idlePot=0xffff;
 assert(joystickHeld(0,idleCia,idlePot)==0);
 // Quadrature: each direction alone, then the diagonals.
 assert(joystickHeld(0x0100,idleCia,idlePot)==1);   // up
 assert(joystickHeld(0x0001,idleCia,idlePot)==2);   // down
 assert(joystickHeld(0x0003,idleCia,idlePot)==4);   // right
 assert(joystickHeld(0x0300,idleCia,idlePot)==8);   // left
 assert(joystickHeld(0x0200,idleCia,idlePot)==9);   // up-left
 assert(joystickHeld(0x0002,idleCia,idlePot)==6);   // down-right
 // Fire is one held bit; the second button is Up.
 assert(joystickHeld(0,0x7f,idlePot)==joystickFire);
 assert(joystickHeld(0,idleCia,0xbfff)==1);
 assert(joystickHeld(0x0100,idleCia,0xbfff)==1);
 // Other CIA and POTINP bits are not controller state.
 assert(joystickHeld(0,0x80,0x4000)==0);
 // Each held raw key reaches the intended Macintosh key position.
 const uint8_t raw[7]={0x4c,0x4d,0x4e,0x4f,fireShift,fireCtrl,fireSpace};
 const uint8_t mac[7]={0x7e,0x7d,0x7c,0x7b,0x38,0x3b,0x31};
 for(unsigned i=0;i<7;++i){KeyTranslation k;assert(translateAmigaKey(raw[i],k)&&k.virtualKey==mac[i]);}
 // Fire follows the original mode and the kid's room.
 Memory m;
 assert(joystickFireKey(m)==fireShift);                       // no room
 m.world[-0x5000]=0xffff;m.world[-0x50bc]=m.world[-0x4fc2]=3;   // play
 assert(joystickFireKey(m)==fireShift);                       // empty room
 m.opponent(0,2,false);m.block[m.room(3)]=1;
 assert(joystickFireKey(m)==fireShift);                       // dead opponent
 m.opponent(1,2,true);
 assert(joystickFireKey(m)==fireShift);                       // beyond the count
 m.block[m.room(3)]=2;
 assert(joystickFireKey(m)==fireCtrl);                        // living opponent
 m.opponent(1,7,true);m.block[m.room(3)+2+38+0x1c]=2;
 assert(joystickFireKey(m)==fireShift);                       // excluded type 7
 m.block[m.room(3)+2+38+0x1c]=1;
 assert(joystickFireKey(m)==fireCtrl);
 m.world[-0x50bc]=0;
 assert(joystickFireKey(m)==fireShift);                       // kid has no room
 m.world[-0x5000]=1;
 assert(joystickFireKey(m)==fireSpace);                       // cinematic
 puts("PASS joystick directions, buttons, key positions and fire selection");
}
