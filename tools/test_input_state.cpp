#include "mac/InputState.h"
#include <cassert>
#include <cstdio>
int main(){
 InputState s;s.reset(0xffff);InputState::Event e;uint8_t keys[16];
 // Press and release within a single consumer interval must both survive.
 assert(s.edge(0,true,'a',17));assert(s.edge(0,false,'a',18));
 assert(!s.down(0));assert(s.queued()==2);
 assert(s.event(8,false,20,e)&&e.what==3&&e.when==17&&e.message==0x20061);
 assert(s.queued()==2);assert(s.event(16,true,20,e)&&e.what==4&&e.when==18);
 assert(s.event(8,true,20,e));assert(!s.event(0xffff,true,23,e)&&e.what==0&&e.when==23);
 // Held polling cannot manufacture extra key-down events.
 assert(s.edge(123,true,0x1c,25));assert(s.edge(123,true,0x1c,26));
 s.keyMap(keys);assert(keys[15]==8);assert(s.queued()==1);
 s.edge(55,true,0,27);s.edge(56,true,0,27);assert(s.modifiers()==0x380&&s.queued()==1);
 s.edge(0,true,'A',28);assert(s.event(8,true,30,e)&&e.message==0x27b1c);
 assert(s.event(8,true,30,e)&&e.message==0x20041&&e.modifiers==0x380);
 s.reset(0xffff);s.edge(0,true,'a',0);s.edge(0,false,'a',1);s.edge(1,true,'s',2);
 s.flush(0xffff,16);assert(s.queued()==2);
 assert(s.event(16,true,3,e)&&e.what==4);assert(s.event(8,true,3,e)&&e.message==0x20173);
 // Overflow is persistent and explicit; levels still reflect the last edge.
 s.reset(0xffff);for(unsigned n=0;n<InputState::capacity;++n)assert(s.edge(0,!(n&1),'a',n));
 assert(!s.edge(0,true,'a',100));assert(s.overflow()&&s.down(0));
 s.flush(0xffff,0);assert(s.overflow()&&s.queued()==0);s.reset(0xffff);assert(!s.overflow());
 puts("PASS input edge retention, masks, peek, flush, modifiers and overflow");
}
