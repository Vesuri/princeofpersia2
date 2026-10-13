#ifdef POP2_AUDIO
#include <proto/exec.h>
#include <exec/memory.h>
#include "PaulaAudio.h"
extern "C" {
uint32_t g_audioDmaCount=0,g_audioDma[4096][14]={};
extern uint32_t g_audioDone;
extern volatile uint16_t g_savedDMA;
uint32_t g_audioSavedADK=0,g_audioActualADK=0;
}
// Register sequencing adapted from the pinned Alone in the Dark MacLoader.
namespace {
uint8_t* buffers[4]={};uint16_t ceiling[4]={65535,65535,65535,65535};
uint16_t master[2]={256,256},volumes[4]={};
bool owned=false;const char* failure=nullptr;
uint16_t savedADK=0;
volatile uint16_t* reg(uint32_t address){return (volatile uint16_t*)address;}
uint16_t beam(){
    uint16_t a,b,h;do{a=*reg(0xdff004);h=*reg(0xdff006);b=*reg(0xdff004);}while((a&1)!=(b&1));
    return ((b&1)<<8)|(h>>8);
}
void waitLines(uint16_t count){uint16_t old=beam();while(count){uint16_t now=beam();if(now!=old){old=now;--count;}}}
void disable(unsigned channel){
    *reg(0xdff096)=1u<<channel;
    waitLines(uint16_t(((uint32_t(ceiling[channel])*2)/227)+2));ceiling[channel]=0;
}
}
namespace PaulaAudio {
const char* error(){return failure;}
bool prepare(){
    failure=nullptr;
    for(auto& p:buffers){p=(uint8_t*)AllocMem(131072,MEMF_CHIP|MEMF_CLEAR);if(!p){failure="AUDIO / CHIP MEMORY";return false;}}
    return true;
}
void release(){for(auto& p:buffers){if(p)FreeMem(p,131072);p=nullptr;}}
uint8_t* buffer(unsigned channel){return channel<4?buffers[channel]:nullptr;}
void volume(unsigned channel,uint16_t value){
    if(channel<4 && value<=64){volumes[channel]=value;if(owned)*reg(0xdff0a8+16*channel)=(value*master[channel==0 || channel==3?0:1]+128)/256;}
}
void masterVolume(uint32_t stereo){master[0]=stereo&65535;master[1]=stereo>>16;for(unsigned c=0;c<4;++c)volume(c,volumes[c]);}
void stop(unsigned channel){
    if(!owned || channel>3)return;
    *reg(0xdff0a8+16*channel)=0;disable(channel);
    *reg(0xdff0a6+16*channel)=124;ceiling[channel]=124;
    *reg(0xdff09c)=0x80u<<channel;*reg(0xdff0aa+16*channel)=0;waitLines(3);
}
void take(){
    if(g_savedDMA&15){failure="AUDIO / ACTIVE SYSTEM DMA";return;}
    savedADK=*reg(0xdff010)&0xff;g_audioSavedADK=savedADK;owned=true;
    // Audio interrupts are not used: a latched silent reload ends one-shots.
    *reg(0xdff09a)=0x0780;*reg(0xdff09e)=0x00ff;
    for(unsigned c=0;c<4;++c)stop(c);
}
void restore(){if(!owned)return;for(unsigned c=0;c<4;++c)stop(c);*reg(0xdff09e)=0xff;*reg(0xdff09e)=0x8000|savedADK;g_audioActualADK=*reg(0xdff010)&0xff;owned=false;}
bool start(unsigned channel,const PaulaSample::Layout& l,uint16_t period,uint16_t volume,uint32_t tailSlot,uint32_t sourceOffset){
    if(!owned || channel>3 || !buffers[channel] || !l.attackBytes || l.attackBytes&1 || !l.reloadBytes || l.reloadBytes&1
       || l.attackBytes>131070 || l.reloadBytes>131070 || l.reloadOffset>131072 || l.reloadBytes>131072-l.reloadOffset
       || period<124 || volume>64){failure="AUDIO / DMA CONTRACT";return false;}
    disable(channel);uint32_t base=0xdff0a0+16*channel;
    *(volatile uint32_t*)base=(uint32_t)buffers[channel];*reg(base+4)=l.attackBytes/2;
    *reg(base+6)=period;ceiling[channel]=period;PaulaAudio::volume(channel,volume);
    *reg(0xdff096)=0x8200|(1u<<channel);waitLines(2);
    *(volatile uint32_t*)base=(uint32_t)(buffers[channel]+l.reloadOffset);*reg(base+4)=l.reloadBytes/2;
#ifdef POP2_AUDIO_PROBE
    if(g_audioDmaCount==4096){failure="AUDIO / DMA TRACE CAPACITY";return false;}
    uint32_t hash=2166136261UL;for(uint32_t i=0;i<l.allocated;++i){hash^=buffers[channel][i];hash*=16777619UL;}
    uint32_t* row=g_audioDma[g_audioDmaCount++];row[0]=g_audioDone;row[1]=channel;row[2]=period;row[3]=volume;
    row[4]=l.attackBytes;row[5]=l.reloadOffset;row[6]=l.reloadBytes;row[7]=l.allocated;row[8]=hash;
    row[9]=*reg(0xdff002);row[10]=TypeOfMem(buffers[channel]);row[11]=*reg(0xdff010)&0xff;row[12]=tailSlot;row[13]=sourceOffset;
#endif
    return true;
}
}
#endif
