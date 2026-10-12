#include "../src/mac/AppleEventHandlers.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
int main() {
 AppleEventHandlers table;int16_t error=123;uint32_t handler=0xcccccccc,ref=0xdddddddd;
 assert(table.lookup(1,2,0,handler,ref,error) && error==-1717 && handler==0xcccccccc && ref==0xdddddddd);
 assert(table.install(1,2,0x1234,0x12345678,0,error) && !error && table.count==1);
 assert(table.install(1,2,0x5678,0xaabbccdd,0,error) && !error && table.count==1);
 assert(table.lookup(1,2,0,handler,ref,error) && !error && handler==0x5678 && ref==0xaabbccdd);
 for(uint32_t bad: {0u,1u,0x1235u}) {
  assert(table.install(1,2,bad,0,0,error) && error==-50 && table.count==1);
  assert(table.lookup(1,2,0,handler,ref,error) && !error && handler==0x5678 && ref==0xaabbccdd);
 }
 for(uint8_t system: {uint8_t(1),uint8_t(2)})assert(!table.install(1,2,0x1234,0,system,error));
 assert(!table.install(0x2a2a2a2a,2,0x1234,0,0,error));
 assert(!table.install(1,0x2a2a2a2a,0x1234,0,0,error));
 assert(!table.lookup(1,2,2,handler,ref,error));
 assert(!table.lookup(0x2a2a2a2a,2,0,handler,ref,error));
 assert(!table.lookup(1,0x2a2a2a2a,0,handler,ref,error));
 assert(table.lookup(1,2,1,handler,ref,error) && error==-1717 && handler==0x5678 && ref==0xaabbccdd);
 for(uint32_t i=1;i<32;++i)assert(table.install(1,i+2,0x1000+i*2,i,0,error) && !error);
 assert(table.count==32 && !table.install(2,1,0x1000,0,0,error));
 assert(table.install(1,2,0x8888,99,0,error) && !error && table.count==32);
 for(uint32_t i=1;i<32;++i)assert(table.lookup(1,i+2,0,handler,ref,error) && !error && handler==0x1000+i*2 && ref==i);
 table.reset();assert(table.count==0 && table.lookup(1,2,0,handler,ref,error) && error==-1717);
 puts("PASS Apple Event handler ownership, replacement, invalid pointers, isolation, capacity and reset");
}
