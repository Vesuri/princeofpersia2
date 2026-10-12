// Takeover/handback order adapted from Alone in the Dark. Display publication
// uses the owned VBI, including temporary DOS service windows.
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/dos.h>
#include <exec/execbase.h>
#include <exec/interrupts.h>
#include <graphics/gfxbase.h>
#include <hardware/dmabits.h>
#include <hardware/intbits.h>
#include "PlatformAmiga.h"
#include "PrinceScreen.h"
#ifdef POP2_INPUT
#include "NativeInput.h"
static uint32_t inputTicks=0;
static uint16_t inputPhase=0;
static void inputField() {
    inputPhase+=6;inputTicks+=inputPhase/5;inputPhase%=5;
    NativeInput::field(inputTicks);
}
#endif
#if defined(POP2_DISPLAY_PROBE) || defined(POP2_DISPLAY)
static PrinceScreen display;
#endif
#if defined(POP2_DISPLAY_PROBE)
static uint8_t displayChunky[640*480];
static uint32_t displayRGB[256];
extern "C" {
volatile uint32_t g_displayProbe=0;
extern volatile uint32_t g_displayFrames;
__attribute__((noinline)) void pop2DisplayProbeComplete() { __asm__ volatile("nop" ::: "memory"); }
}
#endif
#include "SystemWindow.h"
#include "framework/CopperList.h"
#include "framework/AmigaHardware.h"
#ifdef POP2_CORE
#include "CoreRuntime.h"
#include "NativeDisplay.h"
extern "C" void pop2CoreStopped();
#endif
extern DosLibrary* DOSBase;
extern "C" {
volatile uint32_t g_inputVectorRestored=0;
volatile uint32_t g_scaffoldStage=0, g_scaffoldFields=0, g_scaffoldError=0;
volatile uint16_t g_savedDMA=0,g_actualDMA=0,g_savedIRQ=0,g_actualIRQ=0,g_viewRestored=0,g_vectorRestored=0;
__attribute__((noinline)) void pop2ScaffoldRestored() { __asm__ volatile("nop" ::: "memory"); }
}
#ifdef POP2_INPUT_PROBE
extern "C" {
uint32_t g_inputProbeStage=0,g_inputProbeEvents[9][3]={};
uint8_t g_inputProbeKeys[9][16]={};
__attribute__((noinline)) void pop2InputProbeComplete(){__asm__ volatile("nop" ::: "memory");}
}
static void awaitInputField(){uint32_t f=g_scaffoldFields;while(g_scaffoldFields==f){__asm__ volatile("nop" ::: "memory");}}
static void inputProbe(){
    const uint8_t raw[]={0x20,0x4f,0x4e,0x4c,0x4d,0x40,0x44,0x45,0x21};
    for(unsigned i=0;i<9;++i){
        if(i==8){NativeInput::inject(0x67,true);awaitInputField();}
        NativeInput::inject(raw[i],true);awaitInputField();
        NativeInput::keyMap(g_inputProbeKeys[i]);InputState::Event e;
        if(!NativeInput::event(0x18,true,inputTicks,e)||e.what!=3){g_inputProbeStage=0x100+i;break;}
        g_inputProbeEvents[i][0]=e.message;g_inputProbeEvents[i][1]=e.modifiers;g_inputProbeEvents[i][2]=e.when;
        NativeInput::inject(raw[i],false);if(i==8)NativeInput::inject(0x67,false);awaitInputField();
        if(NativeInput::event(0x18,true,inputTicks,e)){g_inputProbeStage=0x200+i;break;}
        g_inputProbeStage=i+1;
    }
    if(NativeInput::error())g_inputProbeStage=0xffff;
    pop2InputProbeComplete();
}
#endif
static Interrupt server;
#ifdef POP2_CORE
static Interrupt windowServer;
static IntVector savedVertb;
static bool vertbTaken=false;
extern "C" uint16_t g_coreServiceActive;
extern "C" { volatile uint32_t g_systemWindows=0,g_systemWindowFields=0;volatile uint16_t g_systemWindowActive=0; }
static uint32_t windowVbi() {
#ifdef POP2_INPUT
    inputField();
#endif
#ifdef POP2_DISPLAY
    display.field();
#endif
    ++g_systemWindowFields;++g_scaffoldFields;return 0; }
// Scheduling and exception-vector handback adapted from Alone in the Dark.
int32_t pop2SystemWindow(int32_t (*operation)(void*),void* context) {
    if(!operation || !vertbTaken || !g_coreServiceActive || g_systemWindowActive ||
       SysBase->TDNestCnt!=0 || SysBase->IDNestCnt!=-1)return -50;
    Disable();
    if(!CoreRuntime::suspendVectors()) { Enable();return -50; }
    uint16_t mask=AmigaHardware::enabledInterrupts();
    IntVector portVertb=SysBase->IntVects[INTB_VERTB];
    SysBase->IntVects[INTB_VERTB]=savedVertb;
    windowServer.is_Node.ln_Type=NT_INTERRUPT;windowServer.is_Node.ln_Pri=127;
    windowServer.is_Node.ln_Name=(char*)"Prince file VBI";
    windowServer.is_Data=0;windowServer.is_Code=(void(*)())windowVbi;
    AddIntServer(INTB_VERTB,&windowServer);
    g_systemWindowActive=1;
    AmigaHardware::setInterrupts(0x7fff,false);
    AmigaHardware::setInterrupts(INTF_INTEN|g_savedIRQ|mask,true);
    Enable();Permit();
    int32_t result=operation(context);
    Forbid();Disable();
    RemIntServer(INTB_VERTB,&windowServer);SysBase->IntVects[INTB_VERTB]=portVertb;
    CoreRuntime::resumeVectors();
    AmigaHardware::setInterrupts(0x7fff,false);
    AmigaHardware::setInterrupts(mask,true);
    g_systemWindowActive=0;++g_systemWindows;
    Enable();return result;
}
#endif
static uint32_t vbi() { *intreqPointer=INTF_VERTB;
#ifdef POP2_INPUT
    inputField();
#endif
#if defined(POP2_DISPLAY_PROBE) || defined(POP2_DISPLAY)
    display.field();
#endif
    ++g_scaffoldFields;return 0; }
static_assert(__builtin_offsetof(ExecBase,IntVects)==84,"Exec vector layout changed");
bool PlatformAmiga::run() {
    g_scaffoldStage=1;
    GfxBase=(struct GfxBase*)OpenLibrary((CONST_STRPTR)"graphics.library",39);
    if(!GfxBase) { g_scaffoldError=1;return false; }
    DOSBase=(DosLibrary*)OpenLibrary((CONST_STRPTR)"dos.library",39);
    if(!DOSBase) { CloseLibrary((Library*)GfxBase);GfxBase=0;g_scaffoldError=2;return false; }
    bool compatible=(SysBase->AttnFlags&AFF_68020) && (GfxBase->ChipRevBits0&GFXF_AA_LISA) && (GfxBase->DisplayFlags&PAL);
    if(!compatible) {
        PutStr((CONST_STRPTR)"SCAFFOLD / REQUIRES PAL AGA 68020\n");
        CloseLibrary((Library*)DOSBase);DOSBase=0;CloseLibrary((Library*)GfxBase);GfxBase=0;g_scaffoldError=3;return false;
    }
#ifdef POP2_CORE
    if(!CoreRuntime::prepare()) {
        PutStr((CONST_STRPTR)CoreRuntime::error());PutStr((CONST_STRPTR)"\n");
        pop2CoreStopped();CoreRuntime::release();
        CloseLibrary((Library*)DOSBase);DOSBase=0;CloseLibrary((Library*)GfxBase);GfxBase=0;
        return false;
    }
#endif
#if defined(POP2_DISPLAY_PROBE) || defined(POP2_DISPLAY)
    if(!display.allocate()) {
        g_scaffoldError=5;
#ifdef POP2_CORE
        CoreRuntime::release();
#endif
        CloseLibrary((Library*)DOSBase);DOSBase=0;CloseLibrary((Library*)GfxBase);GfxBase=0;
        return false;
    }
#endif
    View* view=GfxBase->ActiView;
    CopperList osCopper((uint32_t*)GfxBase->copinit);
    const uint16_t dmaMask=DMAF_ALL|DMAF_MASTER|DMAF_BLITHOG;
    g_savedDMA=AmigaHardware::enabledDMAChannels()&dmaMask;
    g_savedIRQ=AmigaHardware::enabledInterrupts();
#ifdef POP2_INPUT
    inputTicks=0;inputPhase=0;
    if(!NativeInput::initialize()) {
        PutStr((CONST_STRPTR)NativeInput::error());PutStr((CONST_STRPTR)"\n");
#if defined(POP2_DISPLAY_PROBE) || defined(POP2_DISPLAY)
        display.release();
#endif
#ifdef POP2_CORE
        CoreRuntime::release();
#endif
        CloseLibrary((Library*)DOSBase);DOSBase=0;CloseLibrary((Library*)GfxBase);GfxBase=0;return false;
    }
#endif
    LoadView(0);WaitTOF();WaitTOF();
    AmigaHardware::setDMAChannels(DMAF_RASTER|DMAF_SPRITE|DMAF_COPPER,false);
    *bplcon0Pointer=0;
    server.is_Node.ln_Type=NT_INTERRUPT;server.is_Node.ln_Name=(char*)"Prince scaffold VBI";
    server.is_Code=(void(*)())vbi;
    Disable();IntVector saved=SysBase->IntVects[INTB_VERTB];
#ifdef POP2_CORE
    savedVertb=saved;vertbTaken=true;
#endif
    SysBase->IntVects[INTB_VERTB].iv_Data=0;
    SysBase->IntVects[INTB_VERTB].iv_Code=(void(*)())vbi;
    SysBase->IntVects[INTB_VERTB].iv_Node=&server.is_Node;
    Enable();
    Forbid();
    g_scaffoldStage=2;
    AmigaHardware::setInterrupts(INTF_INTEN|INTF_VERTB,true);
#if defined(POP2_DISPLAY_PROBE) || defined(POP2_DISPLAY)
    display.start();
#ifdef POP2_DISPLAY
    NativeDisplay::attach(&display,CoreRuntime::displayTransfer());
#endif
#endif
    while(g_scaffoldFields<4) { __asm__ volatile("nop"); }
#ifdef POP2_DISPLAY_PROBE
    Planar8::Rect viewport={48,64,432,576};
    for(unsigned y=0;y<480;++y)for(unsigned x=0;x<640;++x)
        displayChunky[y*640+x]=uint8_t((x*3+y*7)^(x>>3));
    for(unsigned i=0;i<256;++i)displayRGB[i]=(i<<16)|((255-i)<<8)|(i^0x55);
    for(unsigned step=0;step<4;++step) {
        Planar8::Rect dirty={101,123,139,210};
        if(step==1)for(unsigned y=101;y<139;++y)for(unsigned x=123;x<210;++x)displayChunky[y*640+x]^=0x6d;
        if(step==2)for(unsigned i=0;i<256;++i)displayRGB[i]^=0xa537c1;
        if(step==3) {viewport={47,65,431,577};}
        if(!display.publish(displayChunky,viewport,step==1?&dirty:nullptr,step==1?1:0,displayRGB)) {g_scaffoldError=6;break;}
        while(g_displayFrames<=step) {__asm__ volatile("nop" ::: "memory");}
        uint32_t fields=g_scaffoldFields;
        while(g_scaffoldFields-fields<4) {__asm__ volatile("nop" ::: "memory");}
        g_displayProbe=step+1;pop2DisplayProbeComplete();
    }
#endif
#ifdef POP2_CORE
#ifdef POP2_INPUT_PROBE
    inputProbe();
#endif
#ifdef POP2_INPUT_OVERFLOW
    for(unsigned i=0;i<128;++i)NativeInput::inject(0x20,(i&1)==0);
    {uint32_t field=g_scaffoldFields;while(g_scaffoldFields==field){__asm__ volatile("nop" ::: "memory");}}
#endif
    CoreRuntime::execute();
#endif
#ifdef POP2_INPUT
    NativeInput::shutdown();
#endif
    AmigaHardware::setInterrupts(INTF_VERTB,false);
    AmigaHardware::clearInterruptRequests(INTF_VERTB);
    AmigaHardware::setCopperList(osCopper,true);
    *bplcon3Pointer=0x0c00;
    Disable();SysBase->IntVects[INTB_VERTB]=saved;
#ifdef POP2_CORE
    vertbTaken=false;
#endif
    Enable();
    AmigaHardware::setDMAChannels(dmaMask,false);
    AmigaHardware::setInterrupts(0x7fff,false);
    AmigaHardware::setDMAChannels(g_savedDMA,true);
    AmigaHardware::setInterrupts(g_savedIRQ,true);
#if defined(POP2_DISPLAY_PROBE) || defined(POP2_DISPLAY)
    display.release();
#endif
    Permit();LoadView(view);WaitTOF();WaitTOF();
    g_actualDMA=AmigaHardware::enabledDMAChannels()&dmaMask;
    g_actualIRQ=AmigaHardware::enabledInterrupts();
    g_viewRestored=GfxBase->ActiView==view;
    IntVector& restored=SysBase->IntVects[INTB_VERTB];
    g_vectorRestored=restored.iv_Code==saved.iv_Code && restored.iv_Data==saved.iv_Data && restored.iv_Node==saved.iv_Node;
    bool ok=g_actualDMA==g_savedDMA && g_actualIRQ==g_savedIRQ && g_viewRestored && g_vectorRestored;
#ifdef POP2_INPUT
    ok=ok && g_inputVectorRestored==1;
#endif
    if(!ok)g_scaffoldError=4;
    PutStr((CONST_STRPTR)(ok ? "SCAFFOLD RESTORED\n" : "SCAFFOLD / RESTORE FAILED\n"));
#ifdef POP2_CORE
    PutStr((CONST_STRPTR)CoreRuntime::error());PutStr((CONST_STRPTR)"\n");
    g_scaffoldStage=3;pop2CoreStopped();CoreRuntime::release();
#endif
    CloseLibrary((Library*)DOSBase);DOSBase=0;
    CloseLibrary((Library*)GfxBase);GfxBase=0;
    g_scaffoldStage=3;
    pop2ScaffoldRestored();
#if defined(POP2_FAIL_EXIT) || defined(POP2_CORE)
    return false; // Negative control: the Shell must receive FAIL, not zero.
#else
    return ok;
#endif
}
