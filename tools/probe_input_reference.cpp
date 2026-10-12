#include "mac/InputState.h"
#include "mac/AmigaKeyMap.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>
static uint32_t be(const uint8_t* p,unsigned n){uint32_t v=0;while(n--)v=v*256+*p++;return v;}
int main(int argc,char** argv){
 if(argc!=2){puts("Input reference comparison requires captured samples");return 2;}
 FILE* f=fopen(argv[1],"rb");assert(f);uint8_t b[36];std::vector<uint32_t> messages;std::vector<uint16_t> modifiers;
 bool observed[128]={};uint32_t previous=0;
 while(fread(b,1,36,f)==36){
  uint32_t tick=be(b,4);assert(tick>=previous);previous=tick;
  for(unsigned k=0;k<128;++k)if(b[4+k/8]&(1u<<(k%8)))observed[k]=true;
  unsigned what=be(b+20,2);if(what){assert(what==3);messages.push_back(be(b+22,4));modifiers.push_back(be(b+34,2));assert(be(b+26,4)<=tick);}
 }
 assert(feof(f));fclose(f);assert(messages.size()==9);
 // Authored physical input route: a, arrows, Space, Return, Escape, Amiga-S.
 const uint8_t raw[]={0x20,0x4f,0x4e,0x4c,0x4d,0x40,0x44,0x45,0x21};
 InputState state;InputState::Event event;
 for(unsigned i=0;i<9;++i){
  KeyTranslation key;assert(translateAmigaKey(raw[i],key));assert(observed[key.virtualKey]);
  if(i==8)state.edge(55,true,0,0);
  assert(state.edge(key.virtualKey,true,key.character,100+i));
  assert(state.event(0x18,true,100+i,event));
  assert(event.message==messages[i]&&event.modifiers==modifiers[i]);
  assert(state.edge(key.virtualKey,false,key.character,101+i));
  assert(!state.event(0x18,true,101+i,event));
 }
 assert(observed[55]);puts("PASS original ADB KeyMap and nine keyboard EventRecords, including Command-S");
}
