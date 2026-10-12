#ifdef POP2_UI
#include "NativeUI.h"
#include "NativeInput.h"
#include "mac/MenuRecords.h"
#include <graphics/gfxbase.h>
#include <graphics/text.h>
#include <proto/graphics.h>
extern "C" {
volatile uint32_t g_uiOverlay=0,g_uiOverlayCount=0;
uint8_t g_uiOverlayPixels[640*480];
__attribute__((noinline)) void pop2UIOverlayShown(){__asm__ volatile("nop" ::: "memory");}
}
namespace NativeUI {
namespace {
using H=MacHeap::Handle;
MacHeap* heap;uint8_t* low;NativeDisplay::Valid valid;NativeDisplay::Resource resource;
H menus[4],bar;bool initialized;uint16_t services;const char* reason;
uint16_t w(const uint8_t* p){return uint16_t(p[0])<<8|p[1];}
uint32_t l(const uint8_t* p){return uint32_t(w(p))<<16|w(p+2);}
void sw(uint8_t* p,uint16_t v){p[0]=v>>8;p[1]=v;}
void sl(uint8_t* p,uint32_t v){sw(p,v>>16);sw(p+2,v);}
bool fail(const char* s){reason=s;return false;}
H clone(uint32_t type,int16_t id){
 const uint8_t* bytes;uint32_t size;
 if(!resource(type,id,bytes,size)||size>1024)return nullptr;
 uint8_t saved[1024];for(unsigned i=0;i<size;++i)saved[i]=bytes[i];
 H h=heap->newHandle(size,false);if(!h)return nullptr;
 for(unsigned i=0;i<size;++i)(*h)[i]=saved[i];return h;
}
H owned(uint32_t address){for(H h:menus)if((uint32_t)h==address && h)return h;return nullptr;}
}
bool text(const char* value,unsigned x,unsigned y){
    TextFont* font=GfxBase->DefaultFont;
    if(!font||!font->tf_XSize||font->tf_XSize>16||!font->tf_YSize||font->tf_YSize>16||(font->tf_Flags&FPF_PROPORTIONAL)||!font->tf_CharData||!font->tf_CharLoc)return fail("UI / SYSTEM FONT");
    const uint8_t* bits=(const uint8_t*)font->tf_CharData;
    const uint32_t* locations=(const uint32_t*)font->tf_CharLoc;
    unsigned start=x;
    for(unsigned n=0;value[n];++n){
        uint8_t c=value[n];if(c=='\n'||c=='\r'||x>480){x=start;y+=font->tf_YSize+4;if(c=='\n'||c=='\r')continue;}
        if(y>360)return fail("UI / TEXT EXTENT");
        if(c<font->tf_LoChar||c>font->tf_HiChar)c='?';
        uint32_t location=locations[c-font->tf_LoChar];unsigned offset=location>>16,width=location&65535;
        if(width>font->tf_XSize || offset+width>font->tf_Modulo*8)return fail("UI / FONT EXTENT");
        for(unsigned row=0;row<font->tf_YSize;++row)for(unsigned col=0;col<width;++col)
            if(bits[row*font->tf_Modulo+(offset+col)/8]&(128u>>((offset+col)&7)))g_uiOverlayPixels[(y+row)*640+x+col]=0;
        x+=font->tf_XSize;
    }
    return true;
}
bool replacement(uint16_t selector,uint32_t* regs,uint8_t* args){
    char message[513];uint16_t kind=0;
    if(selector==1){
        if(!valid((uint32_t)args,2)||w(args)!=600)return fail("UI / ABOUT ID");
        const char* body="Amiga port\n\nOriginal game by Jordan Mechner\nPublished by Broderbund\n\nAmiga + I: About   Amiga + P: Pause\nAmiga + N/O/S: New / Open / Save\nAmiga + T/M: Sound / Music\nAmiga + H/R/Q: Hall of Fame / Restart / Quit";
        unsigned i=0;do{message[i]=body[i];}while(body[i++]);
    }else if(selector==2){
        if(!valid((uint32_t)args,6))return fail("UI / ALERT ARGUMENTS");
        kind=w(args);if(kind!=401&&kind!=404&&kind!=405&&kind!=406)return fail("UI / ALERT ID");
        uint32_t source=l(args+2);unsigned n=0;
        do{if(n==512||!valid(source+n,1))return fail("UI / ALERT TEXT");message[n]=((const char*)source)[n];}while(message[n++]);
    }else return fail("UI / UNKNOWN REPLACEMENT");
    for(auto& pixel:g_uiOverlayPixels)pixel=255;
    if(!text(selector==1?"PRINCE OF PERSIA 2":"PRINCE OF PERSIA 2 - MESSAGE",32,40)||!text(message,32,80)||
       !text(kind==406?"Y / Return: Yes       N / Escape: No":kind==405?"Return: OK       Escape: Cancel":"Return / Escape: OK",32,328))return false;
    NativeInput::flush(0xffff,0);g_uiOverlay=selector==1?600:kind;
    if(!NativeDisplay::showOverlay(g_uiOverlayPixels))return fail(NativeDisplay::error());
    ++g_uiOverlayCount;pop2UIOverlayShown();
    uint16_t answer=0;
    while(!answer){
        if(NativeInput::error())return fail(NativeInput::error());
        InputState::Event event;
        if(!NativeInput::event(8,true,NativeInput::ticks(),event))continue;
        uint8_t key=event.message;
        if(key==13||((key=='y'||key=='Y')&&kind==406))answer=1;
        if(key==27||((key=='n'||key=='N')&&kind==406))answer=(kind==405||kind==406)?2:1;
    }
    g_uiOverlay=0;NativeInput::flush(0xffff,0);
    if(!NativeDisplay::hideOverlay())return fail(NativeDisplay::error());
    regs[0]=answer;return true;
}
const char* error(){return reason;}
bool prepare(MacHeap& owner,uint8_t* shadow,NativeDisplay::Valid validator,NativeDisplay::Resource reader){
 heap=&owner;low=shadow;valid=validator;resource=reader;bar=nullptr;initialized=false;services=0;
 for(auto& h:menus)h=nullptr;sw(low+0x144,0xffef);reason="UI / READY";return true;
}
bool handles(uint16_t t){
 switch(t){case 0xa8fe:case 0xa912:case 0xa930:case 0xa9cc:case 0xa97b:case 0xa850:case 0xa852:case 0xa853:
 case 0xa856:case 0xa063:case 0xa032:case 0xa9c0:case 0xa93c:case 0xa949:case 0xa94d:case 0xa937:
 case 0xa976:case 0xa970:case 0xa971:case 0xa860:case 0xa938:case 0xa939:case 0xa93a:case 0xa944:case 0xa945:case 0xa946:case 0xa947:case 0xa93e:return true;default:return false;}
}
bool dispatch(uint16_t t,uint32_t* regs,uint8_t* a,uint32_t& consumed){
 consumed=0;
 switch(t){
 // D8: owned UI has no Macintosh fonts, TextEdit objects or cursor pixels.
 // Initialization records readiness; later unsupported operations remain stops.
 case 0xa8fe:services|=1;return true;
 case 0xa912:if(!NativeDisplay::initWindowManager())return fail(NativeDisplay::error());services|=2;return true;
 case 0xa930:services|=4;return true;
 case 0xa9cc:services|=8;return true;
 case 0xa97b:
  if(!valid((uint32_t)a,4)||l(a))return fail("UI / INITDIALOGS CALLBACK");
  services|=16;consumed=4;return true;
 case 0xa850:case 0xa852:case 0xa853:case 0xa856:return true;
 case 0xa063:regs[0]=0;return true; // The complete owned application zone is already reserved.
 case 0xa032:NativeInput::flush(uint16_t(regs[0]),regs[0]>>16);regs[0]=0;return true;
 case 0xa9c0:{
  if(services!=31||!valid((uint32_t)a,6)||w(a)!=400||bar)return fail("UI / GETMBAR FORM");
  bar=clone(0x4d424152,400);if(!bar||heap->handleSize(bar)<10||w(*bar)!=4)return fail("UI / MBAR CONTENTS");
  for(unsigned i=0;i<4;++i){
   if(w(*bar+2+i*2)!=400+i)return fail("UI / MENU ORDER");
   menus[i]=clone(0x4d454e55,400+i);MenuRecords::Item item;uint16_t count;
   if(!menus[i]||!MenuRecords::scan(*menus[i],heap->handleSize(menus[i]),0,item,count))return fail("UI / MENU RECORD");
  }
  // D9: no command can enable the unsupported in-place compression action.
  sl(*menus[3]+10,l(*menus[3]+10)&~uint32_t((1u<<7)|(1u<<4)));
  sl(a+2,(uint32_t)bar);consumed=2;return true;
 }
 case 0xa93c:
  if(!valid((uint32_t)a,4)||l(a)!=(uint32_t)bar||!bar)return fail("UI / SETMENUBAR OWNER");
  initialized=true;consumed=4;return true;
 case 0xa949:
  if(!initialized||!valid((uint32_t)a,6)||w(a)<400||w(a)>403)return fail("UI / MENUHANDLE ID");
  sl(a+2,(uint32_t)menus[w(a)-400]);consumed=2;return true;
 case 0xa94d:
  if(!valid((uint32_t)a,8)||l(a)!=0x44525652||l(a+4)!=(uint32_t)menus[0])return fail("UI / ADDRESMENU FORM");
  consumed=8;return true; // D8: no desk accessories in the port's action list.
 case 0xa937:if(!initialized)return fail("UI / DRAW BEFORE INIT");return true;
 case 0xa938:if(!valid((uint32_t)a,2)||w(a))return fail("UI / HILITE MENU");consumed=2;return true;
 case 0xa976:
  if(!valid((uint32_t)a,4)||!valid(l(a),16))return fail("INPUT / GETKEYS OUTPUT");
  NativeInput::keyMap((uint8_t*)l(a));consumed=4;return true;
 case 0xa970:case 0xa971:case 0xa860:{
  bool wait=t==0xa860;uint32_t size=wait?14:6;
  if(!valid((uint32_t)a,size+2)||!valid(l(a+(wait?8:0)),16))return fail("INPUT / EVENT ARGUMENTS");
  if(wait && (l(a)||l(a+4)))return fail("INPUT / UNMEASURED WAIT FORM");
  uint8_t* out=(uint8_t*)l(a+(wait?8:0));InputState::Event event;
  bool found=NativeInput::event(w(a+(wait?12:4)),t!=0xa971,NativeInput::ticks(),event);
  sw(out,event.what);sl(out+2,event.message);sl(out+6,event.when);
  sw(out+10,event.v);sw(out+12,event.h);sw(out+14,event.modifiers);
  a[size]=found?1:0;consumed=size;return true;
 }
 case 0xa944:case 0xa945:{
  if(!valid((uint32_t)a,8))return fail("UI / MARK ARGUMENTS");
  H h=owned(l(a+4));MenuRecords::Item item;uint16_t count;
  if(!h||!w(a+2)||!MenuRecords::scan(*h,heap->handleSize(h),w(a+2),item,count))return fail("UI / MARK ITEM");
  uint8_t mark=t==0xa945?(a[0]?0x12:0):uint8_t(w(a));
  if(h==menus[3]&&w(a+2)==7)mark=0;
  if(h==menus[3]&&w(a+2)==4)mark=0x12;
  (*h)[item.offset+item.length+3]=mark;consumed=8;return true;
 }
 case 0xa946:case 0xa947:{
  if(!valid((uint32_t)a,10))return fail("UI / TEXT ARGUMENTS");
  H h=owned(l(a+6));uint8_t* text=(uint8_t*)l(a);
  if(!h||!valid((uint32_t)text,256))return fail("UI / TEXT OWNER");
  if(t==0xa946){if(!MenuRecords::get(*h,heap->handleSize(h),w(a+4),text))return fail("UI / GET ITEM TEXT");}
  else{
   uint8_t stable[256];for(unsigned i=0;i<=text[0];++i)stable[i]=text[i];
   MenuRecords::Item item;uint32_t size=heap->handleSize(h),next;
   if(!MenuRecords::replacement(*h,size,w(a+4),stable[0],item,next))return fail("UI / SET ITEM TEXT");
   if(next>size && heap->setHandleSize(h,next))return fail("UI / MENU GROW");
   MenuRecords::replace(*h,size,item,stable);
   if(next<size && heap->setHandleSize(h,next))return fail("UI / MENU SHRINK");
  }
  consumed=10;return true;
 }
 case 0xa93e:{
  if(!initialized||!valid((uint32_t)a,6))return fail("UI / MENU KEY ARGUMENTS");
  uint8_t key=w(a);if(key>='a'&&key<='z')key-=32;uint32_t choice=0;
  for(H h:menus){
   if(!(l(*h+10)&1))continue;
   MenuRecords::Item item;uint16_t count;
   if(!MenuRecords::scan(*h,heap->handleSize(h),0,item,count))return fail("UI / MENU KEY RECORD");
   for(unsigned n=1;n<=count;++n){
    if(n>31)return fail("UI / MENU KEY COUNT");
    if(!MenuRecords::scan(*h,heap->handleSize(h),n,item,count))return fail("UI / MENU KEY ITEM");
    if(key && (*h)[item.offset+item.length+2]==key && (l(*h+10)&(uint32_t(1)<<n)))choice=uint32_t(w(*h))<<16|n;
   }
  }
  if(key=='I' && (l(*menus[0]+10)&3)==3)choice=0x01900001;
  if(key=='P' && (l(*menus[3]+10)&3)==3)choice=0x01930001;
  sl(a+2,choice);consumed=2;return true;
 }
 case 0xa939:case 0xa93a:{
  if(!valid((uint32_t)a,6))return fail("UI / ITEM ARGUMENTS");
  H h=owned(l(a+2));uint16_t item=w(a);MenuRecords::Item record;uint16_t count;
  if(!h||item>31||!MenuRecords::scan(*h,heap->handleSize(h),item,record,count))return fail("UI / ITEM OWNER");
  uint32_t flags=l(*h+10),bit=uint32_t(1)<<item;
  if(t==0xa939 && !(h==menus[3] && (item==7||item==4)))flags|=bit;else flags&=~bit;
  sl(*h+10,flags);consumed=6;return true;
 }
 default:return fail("UI / UNKNOWN SERVICE");
 }
}
}
#endif
