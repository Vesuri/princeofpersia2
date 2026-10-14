#ifdef POP2_INPUT
#include <proto/exec.h>
#include <exec/interrupts.h>
#include <resources/cia.h>
#include <proto/cia.h>
#include <hardware/cia.h>
#include "NativeInput.h"
#include "mac/AmigaKeyMap.h"
#include "framework/AmigaHardware.h"
extern "C" {extern volatile uint32_t g_inputVectorRestored;}
// CIA ownership/handshake adapted from the pinned Alone in the Dark driver.
// Only the VBI translates queued edges into Macintosh levels and events.
#ifdef POP2_UI_PROBE
extern "C" {extern volatile uint32_t g_uiOverlay,g_uiOverlayCount;extern volatile uint8_t g_uiProbeReplyRaw;}
#endif
#ifdef POP2_GAMEPLAY_HALL_PROBE
#if !defined(POP2_VERIFY) || !defined(POP2_GAMEPLAY_ROUTE_PROBE)
#error Hall input requires explicit route verification
#endif
extern "C" {extern volatile uint32_t g_uiOverlay,g_uiOverlayCount;}
#endif
#ifdef POP2_INTEGRATION_PROBE
extern "C" {extern volatile uint32_t g_integrationStage,g_integrationRightAt;volatile uint32_t g_integrationEscReads=0,g_integrationRightReads=0,g_integrationSpaceEvents=0,g_integrationKeys[4]={};volatile uint32_t g_integrationPulseRaw=128,g_integrationPulseStart=0,g_integrationPulseEnd=0;}
#endif
namespace NativeInput {
namespace {
Library* cia;Interrupt handler;Interrupt* previous;
volatile uint8_t rawQueue[128],head,tail;volatile bool rawOverflow;
bool physical[128];InputState state;const char* reason;volatile uint32_t now;
void enqueue(uint8_t raw,bool down) {
 const uint8_t next=(head+1)&127;
 if(next==tail){rawOverflow=true;return;}
 rawQueue[head]=raw|(down?0:128);head=next;
}
uint32_t keyboard() {
 uint8_t code=uint8_t(~*ciaasdrPointer);
 *ciaacraPointer|=CIACRAF_SPMODE;
 for(volatile uint16_t delay=0;delay<200;++delay){}
 *ciaacraPointer&=uint8_t(~CIACRAF_SPMODE);
 code=(code>>1)|(code<<7);enqueue(code&127,!(code&128));return 0;
}
}
bool initialize() {
 if(cia){reason="INPUT / DUPLICATE INITIALIZE";return false;}
 g_inputVectorRestored=0;state.reset();now=0;head=tail=0;rawOverflow=false;reason=nullptr;
 for(auto& v:physical)v=false;
 cia=(Library*)OpenResource((CONST_STRPTR)CIAANAME);
 if(!cia){reason="INPUT / CIA RESOURCE";return false;}
 handler.is_Node.ln_Type=NT_INTERRUPT;handler.is_Node.ln_Pri=0;
 handler.is_Node.ln_Name=(char*)"Prince keyboard";handler.is_Data=0;
 handler.is_Code=(void(*)())keyboard;
 Disable();previous=AddICRVector(cia,CIAICRB_SP,&handler);
 if(previous){RemICRVector(cia,CIAICRB_SP,previous);AddICRVector(cia,CIAICRB_SP,&handler);}
 Enable();return true;
}
void shutdown() {
 if(!cia)return;
 Disable();RemICRVector(cia,CIAICRB_SP,&handler);
 if(previous)AddICRVector(cia,CIAICRB_SP,previous);
 // AddICRVector reports an occupied slot without replacing it. Probe the
 // restored owner; if it was empty, remove our temporary probe immediately.
 Interrupt* restored=AddICRVector(cia,CIAICRB_SP,&handler);
 g_inputVectorRestored=restored==previous;
 if(!restored)RemICRVector(cia,CIAICRB_SP,&handler);
 if(!g_inputVectorRestored)reason="INPUT / CIA VECTOR RESTORE";
 cia=nullptr;previous=nullptr;Enable();
}
uint32_t ticks(){return now;}
void field(uint32_t ticks) {
 now=ticks;
 if(!cia || reason)return;
#ifdef POP2_UI_PROBE
 static uint32_t replied=0;static uint8_t held=0xff;
 if(held!=0xff){enqueue(held,false);held=0xff;}
 if(g_uiOverlay && g_uiOverlayCount && replied!=g_uiOverlayCount){
     replied=g_uiOverlayCount;held=g_uiProbeReplyRaw;enqueue(held,true);
 }
#endif
#ifdef POP2_INTEGRATION_PROBE
#ifdef POP2_GAMEPLAY_HALL_PROBE
 static uint32_t hallSeen=0,hallKey=0;
 static const uint8_t hallKeys[]={0x33,0x18,0x22,0x12,0x32,0x44,0x44};
 if((g_uiOverlay==7003 || g_uiOverlay==7004) && g_uiOverlayCount!=hallSeen){
  hallSeen=g_uiOverlayCount;
  if(hallKey<sizeof hallKeys){
   g_integrationPulseRaw=hallKeys[hallKey++];g_integrationPulseStart=ticks+6;g_integrationPulseEnd=ticks+10;
  }
 }
#endif
 static uint32_t observed[4]={};
 for(unsigned group=0;group<4;++group){
  uint32_t keys=g_integrationKeys[group],pulse=g_integrationPulseRaw;
  if(pulse<128 && pulse/32==group && int32_t(ticks-g_integrationPulseStart)>=0 && int32_t(g_integrationPulseEnd-ticks)>0)keys|=1UL<<(pulse%32);
#ifdef POP2_GAMEPLAY_RESTART_PROBE
  if(pulse==0x13 && group==3 && int32_t(ticks-(g_integrationPulseStart-2))>=0 && int32_t(g_integrationPulseEnd-ticks)>0)keys|=1UL<<(0x66%32);
#endif
#if !defined(POP2_INTEGRATION_NO_RIGHT) && !defined(POP2_GAMEPLAY_RESTART_PROBE) && !defined(POP2_GAMEPLAY_ROUTE_PROBE)
  if(g_integrationStage==3 && int32_t(ticks-g_integrationRightAt)>=0 && group==2)keys|=1UL<<(0x4e%32);
#endif
  uint32_t changed=keys^observed[group];
  for(unsigned bit=0;bit<32;++bit)if(changed&(1UL<<bit))enqueue(group*32+bit,(keys&(1UL<<bit))!=0);
  observed[group]=keys;
 }
#endif
 if(rawOverflow){reason="INPUT / RAW QUEUE OVERFLOW";return;}
 while(tail!=head) {
  uint8_t edge=rawQueue[tail];tail=(tail+1)&127;
  uint8_t raw=edge&127;bool down=!(edge&128);
  if(physical[raw]==down)continue;
  physical[raw]=down;KeyTranslation key;
  if(!translateAmigaKey(raw,key)){reason="INPUT / UNMAPPED RAW KEY";return;}
  // Both physical Shifts/Alts/Amigas can share one Macintosh key position.
  bool held=false;
  for(unsigned i=0;i<128;++i)if(physical[i]) {
   KeyTranslation alias;if(translateAmigaKey(i,alias)&&alias.virtualKey==key.virtualKey){held=true;break;}
  }
  bool shift=(state.modifiers()&0x200)!=0;
  bool caps=(state.modifiers()&0x400)!=0 && key.character>='a' && key.character<='z';
  uint8_t character=(shift||caps)?key.shiftedCharacter:key.character;
  if(!state.edge(key.virtualKey,held,character,ticks)){reason="INPUT / EVENT QUEUE OVERFLOW";return;}
 }
}
void keyMap(uint8_t* output){
 Disable();state.keyMap(output);
#ifdef POP2_INTEGRATION_PROBE
 if(g_integrationStage==3 && (output[15]&0x10))++g_integrationRightReads;
 if(output[6]&0x20)++g_integrationEscReads;
#endif
 Enable();
}
bool event(uint16_t mask,bool remove,uint32_t ticks,InputState::Event& output){
 Disable();bool found=state.event(mask,remove,ticks,output);
#ifdef POP2_INTEGRATION_PROBE
 if(g_integrationStage==2 && found && remove && output.what==3 && (output.message&65535)==0x3120)++g_integrationSpaceEvents;
#endif
 Enable();return found;
}
void flush(uint16_t which,uint16_t stop){Disable();state.flush(which,stop);Enable();}
const char* error(){return reason;}
void inject(uint8_t raw,bool down){Disable();if(raw<128)enqueue(raw,down);else reason="INPUT / INJECT KEY RANGE";Enable();}
}
#endif
