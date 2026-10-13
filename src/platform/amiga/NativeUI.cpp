#ifdef POP2_UI
#include "NativeUI.h"
#include "NativeInput.h"
#include "NativeFiles.h"
#include "mac/MenuRecords.h"
#include <graphics/gfxbase.h>
#include <graphics/text.h>
#include <proto/graphics.h>
extern "C" {
volatile uint32_t g_uiOverlay=0,g_uiOverlayCount=0;
uint8_t g_uiOverlayPixels[640*480];
__attribute__((noinline)) void pop2UIOverlayShown(){__asm__ volatile("nop" ::: "memory");}
}
#ifdef POP2_GAMEPLAY_SAVE_PROBE
extern "C" void pop2GameplayPickerShown();
#endif
namespace NativeUI {
namespace {
using H=MacHeap::Handle;
MacHeap* heap;uint8_t* low;NativeDisplay::Valid valid;NativeDisplay::Resource resource;
H menus[4],bar,cursor;bool initialized;uint16_t services;const char* reason;
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
// D8 owns the file chooser. Both donor ports use game-specific save UI;
// neither implements this Standard File call. Reuse our existing overlay,
// input queue and shared MacFiles directory/metadata model.
bool fileDialog(uint8_t* a,uint32_t& consumed){
    if(!valid((uint32_t)a,2))return fail("UI / FILE SELECTOR EXTENT");
    uint16_t selector=w(a);bool saving=selector==1;
    if(!saving && selector!=2)return fail("UI / FILE SELECTOR");
    uint32_t bytes=saving?22:28;
    if(!valid((uint32_t)a,bytes)||!valid(l(a+2),74)||l(a+6))return fail("UI / FILE ARGUMENTS");
    if(!saving && (w(a+14)!=1 || l(a+16) || !valid(l(a+10),4) || l((uint8_t*)l(a+10))!=0x53415645))return fail("UI / FILE FILTER");
    char name[32]={};unsigned length=0;
    if(saving){
        uint8_t* initial=(uint8_t*)l(a+10);
        if(!valid((uint32_t)initial,1) || initial[0]>31 || !valid((uint32_t)initial,initial[0]+1))return fail("UI / FILE DEFAULT NAME");
        length=initial[0];for(unsigned i=0;i<length;++i)name[i]=initial[i+1];
    }
    MacFiles& catalog=NativeFiles::catalog();uint32_t choices[MacFiles::maxEntries];unsigned count=0,selected=0;
    if(!saving)for(int16_t n=1;n<=MacFiles::maxEntries;++n){
        uint32_t id=0;int16_t error=catalog.indexedFile(MacFiles::applicationWD,catalog.application,n,id);
        if(error==MacFiles::fnfErr)break;if(error)return fail("UI / FILE DIRECTORY");
        const auto* entry=catalog.entry(id);
        if(!entry || !entry->metadataKnown)return fail("UI / FILE METADATA");
        if(l(entry->metadata.finder)==0x53415645)choices[count++]=id;
    }
    NativeInput::flush(0xffff,0);bool dirty=true,accepted=false,done=false,overwrite=false;
#ifdef POP2_GAMEPLAY_SAVE_PROBE
    pop2GameplayPickerShown();
#endif
    while(!done){
        if(dirty){
            for(auto& pixel:g_uiOverlayPixels)pixel=255;
            if(!text(saving?"SAVE GAME":"OPEN GAME",32,40))return false;
            if(saving){
                if(!text("Name:",32,88)||!text(name,32,112)||
                   !text(overwrite?"Replace existing save? Return: Yes / Escape: No":"Type a name. Backspace deletes.",32,160))return false;
            }else{
                if(!count && !text("No saved games.",32,88))return false;
                unsigned page=(selected/10)*10;
                for(unsigned i=page;i<count && i<page+10;++i){
                    if(i==selected && !text(">",32,80+(i-page)*20))return false;
                    if(!text(catalog.entry(choices[i])->name,48,80+(i-page)*20))return false;
                }
            }
            if(!text(saving?"Return: Save       Escape: Cancel":"Up/Down: Select   Return: Open   Esc: Cancel",32,328))return false;
            if(!NativeDisplay::showOverlay(g_uiOverlayPixels))return fail(NativeDisplay::error());
            g_uiOverlay=saving?7001:7002;++g_uiOverlayCount;pop2UIOverlayShown();dirty=false;
        }
        if(NativeInput::error())return fail(NativeInput::error());
        InputState::Event event;if(!NativeInput::event(8,true,NativeInput::ticks(),event))continue;
        uint8_t key=event.message;
        if(key==27){if(overwrite){overwrite=false;dirty=true;}else done=true;continue;}
        if(key==13){
            if(!saving){if(count){accepted=done=true;}continue;}
            if(!length)continue;
            const auto* existing=catalog.child(catalog.application,name);
            if(existing){
                // Only save documents may be replaced; installed assets remain immutable.
                if(existing->directory || !existing->metadataKnown || l(existing->metadata.finder)!=0x53415645)continue;
                if(!overwrite){overwrite=true;dirty=true;continue;}
            }else{MacFiles::Entry candidate;if(catalog.planCreate(MacFiles::applicationWD,catalog.application,name,candidate))return fail("UI / SAVE DESTINATION");}
            accepted=done=true;continue;
        }
        if(saving && !overwrite){
            if(key==8 && length){name[--length]=0;dirty=true;}
            else if(key>=32 && key<127 && key!=':' && key!='/' && length<31){name[length++]=key;name[length]=0;dirty=true;}
        }else if(!saving && count){
            if(key==30 && selected){--selected;dirty=true;}
            if(key==31 && selected+1<count){++selected;dirty=true;}
        }
    }
    uint8_t* out=(uint8_t*)l(a+2);out[0]=accepted?1:0;
    if(accepted){
        for(unsigned i=1;i<74;++i)out[i]=0;
        if(!saving){const auto* entry=catalog.entry(choices[selected]);length=0;while(entry->name[length]){name[length]=entry->name[length];++length;}sl(out+2,0x53415645);}
        sw(out+6,MacFiles::applicationWD);out[10]=length;for(unsigned i=0;i<length;++i)out[11+i]=name[i];
    }
    g_uiOverlay=0;NativeInput::flush(0xffff,0);
    if(!NativeDisplay::hideOverlay())return fail(NativeDisplay::error());
    consumed=bytes;return true;
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
 heap=&owner;low=shadow;valid=validator;resource=reader;bar=cursor=nullptr;initialized=false;services=0;
 for(auto& h:menus)h=nullptr;sw(low+0x144,0xffef);reason="UI / READY";return true;
}
bool handles(uint16_t t){
 switch(t){case 0xa8fe:case 0xa912:case 0xa930:case 0xa9cc:case 0xa97b:case 0xa850:case 0xa852:case 0xa853:
 case 0xa851:case 0xa9b9:case 0xa856:case 0xa063:case 0xa032:case 0xa9c0:case 0xa93c:case 0xa949:case 0xa94d:case 0xa937:
 case 0xa972:case 0xa974:case 0xa976:case 0xa970:case 0xa971:case 0xa860:case 0xa938:case 0xa939:case 0xa93a:case 0xa944:case 0xa945:case 0xa946:case 0xa947:case 0xa93e:case 0xa9ea:return true;default:return false;}
}
bool dispatch(uint16_t t,uint32_t* regs,uint8_t* a,uint32_t& consumed){
 consumed=0;
 switch(t){
 case 0xa9ea:return fileDialog(a,consumed);
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
 // D8 removes the pointer. Retain the reached wait-cursor handle/copy ABI
 // with an owned transparent cursor, so original setup and locking still run.
 case 0xa9b9:
  if(!valid((uint32_t)a,6)||w(a)!=4)return fail("UI / CURSOR ID");
  if(!cursor){cursor=heap->newHandle(68,true);if(!cursor)return fail("UI / CURSOR ALLOCATION");}
  if(!heap->isHandle(cursor)||heap->handleSize(cursor)!=68)return fail("UI / CURSOR OWNER");
  sl(a+2,(uint32_t)cursor);sw(low+0xa60,0);consumed=2;regs[0]=0;return true;
 case 0xa851:
  if(!valid((uint32_t)a,4)||!valid(l(a),68))return fail("UI / CURSOR EXTENT");
  consumed=4;return true;
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
 // GetMouse follows the same input position as Event Manager, as in both
 // reference ports. D8 removes the pointer; the owned input model stays fixed.
 case 0xa972:{
  if(!valid((uint32_t)a,4)||!valid(l(a),4))return fail("INPUT / GETMOUSE OUTPUT");
  InputState::Event event;NativeInput::event(0,false,NativeInput::ticks(),event);
  uint8_t* out=(uint8_t*)l(a);sw(out,event.v);sw(out+2,event.h);
  consumed=4;return true;
 }
 // Button uses the same mouse-up state as Event Manager and GetMouse.
 // Both reference ports return a Boolean in the caller's two-byte slot.
 case 0xa974:{
  if(!valid((uint32_t)a,2))return fail("INPUT / BUTTON OUTPUT");
  InputState::Event event;NativeInput::event(0,false,NativeInput::ticks(),event);
  a[0]=(event.modifiers&0x80)?0:1;a[1]=0;return true;
 }
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
