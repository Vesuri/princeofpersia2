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
#include "mac/BlitterCompletion.h"
#ifdef POP2_AUDIO
#include "NativeAudio.h"
#endif
#ifdef POP2_INPUT
#include "NativeInput.h"
#include "mac/VideoTicks.h"
static volatile uint32_t inputTicks=0;
static volatile uint16_t inputPhase=0;
static void inputField() {
    uint16_t phase=inputPhase;
    uint16_t delta=VideoTicks::fieldDelta(true,phase);
    inputPhase=phase;inputTicks+=delta;
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
#ifdef POP2_CORE
extern uint32_t g_coreStopTrap,g_coreStopSegment,g_coreStopOffset;
#endif
}
static const char* failure=0;
static char failureText[160];
#ifdef POP2_CORE
// The core reason, with the original location of a named stop.
static const char* coreFailure() {
    const char* why=CoreRuntime::error();
    if(!g_coreStopSegment)return why;
    char* out=failureText;char* end=failureText+sizeof failureText-1;
    auto text=[&](const char* p){while(*p && out<end)*out++=*p++;};
    auto hex=[&](uint32_t v){char digits[9];int n=0;do{digits[n++]="0123456789ABCDEF"[v&15];v>>=4;}while(v);while(n && out<end)*out++=digits[--n];};
    text(why);text(" (trap $");hex(g_coreStopTrap);text(" at CODE ");
    {char digits[11];int n=0;uint32_t v=g_coreStopSegment;do{digits[n++]=char('0'+v%10);v/=10;}while(v);while(n && out<end)*out++=digits[--n];}
    text("+$");hex(g_coreStopOffset);text(")");*out=0;
    return failureText;
}
#endif
const char* PlatformAmiga::reason() const { return failure; }
#ifdef POP2_INPUT
// Called at user-mode service points. Pair the field clock with a stable beam
// and pending-VBI state: a request not yet serviced belongs to the next field.
extern "C" { volatile uint32_t g_clockFault=0,g_clockFaultData[5]={}; }
static uint32_t lastWallTick=0;
// As in the other ports, ordinary scheduler checks use the interrupt epoch.
// A precise TickCount sample may already have observed this field's fractional
// tick; never regress that observation before the next interrupt catches up.
extern "C" uint32_t pop2ServiceTicks() {
    uint32_t base=inputTicks;
    return int32_t(base-lastWallTick)>0 ? base : lastWallTick;
}
extern "C" uint32_t pop2WallTicks() {
    uint32_t& lastTick=lastWallTick;
    static uint32_t settledField=0;
    static bool settled=false;
    // Once the sole sub-field tick has occurred, this field cannot advance
    // again. Check the interrupt request, not the beam; a pending VBI or a
    // changed epoch takes the full ambiguity/monotonicity checks below.
    uint32_t epoch=g_scaffoldFields;
    if(settled && epoch==settledField && !(*intreqrPointer&INTF_VERTB) &&
       epoch==g_scaffoldFields)return lastTick;
    uint32_t before,after,base;uint16_t phase,hi,lo,check,pending,again;
    do {
        before=g_scaffoldFields;base=inputTicks;phase=inputPhase;
        pending=*intreqrPointer&INTF_VERTB;
        hi=*vposrPointer;lo=*vhposrPointer;check=*vposrPointer;
        again=*intreqrPointer&INTF_VERTB;after=g_scaffoldFields;
    // The beam can expose scanline zero before the VBI request/epoch catches
        // up. HP 0/1 can likewise still carry the preceding vertical position.
    } while(before!=after || pending!=again || ((hi^check)&1) ||
            (lo&255)<2 || (((hi&1)<<8)|(lo>>8))==0);
    if(pending)base+=VideoTicks::fieldDelta(true,phase);
    uint32_t line=((hi&1)<<8)|(lo>>8);
    uint32_t tick;
    if((lo&255)<227)tick=VideoTicks::withinPALBeam(base,phase,uint16_t(line),uint16_t(lo&255));
    else {
        uint32_t micro=line*64+((lo&255)*64)/227;
        tick=VideoTicks::withinPALField(base,phase,uint16_t(micro<20000?micro:19999));
    }
    settled=!pending && tick==base+1;
    settledField=after;
    if(int32_t(tick-lastTick)<0 && !g_clockFault) {
        g_clockFaultData[0]=lastTick;g_clockFaultData[1]=tick;
        g_clockFaultData[2]=after;g_clockFaultData[3]=(uint32_t(hi)<<16)|lo;
        g_clockFaultData[4]=(uint32_t(phase)<<16)|pending;g_clockFault=1;
    }
    lastTick=tick;return tick;
}
#endif
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
#if defined(POP2_TIMING_PROBE) && defined(POP2_INTEGRATION_PROBE)
extern "C" {extern volatile uint32_t g_timingPhase,g_timingSamples[0x1005],g_integrationStage;}
static void timingSample() {if(g_integrationStage==3 && g_timingPhase<0x1005)++g_timingSamples[g_timingPhase];}
#else
static void timingSample() {}
#endif
static Interrupt server;
#if defined(POP2_DISPLAY_PROBE) || defined(POP2_DISPLAY)
static Interrupt blitServer;
static IntVector savedBlit;
#ifdef POP2_DISPLAY_PROBE
extern "C" {volatile uint32_t g_displayCopyInterrupts=0;}
#endif
static uint32_t blitComplete() {
#ifdef POP2_DISPLAY_PROBE
    ++g_displayCopyInterrupts;
#endif
    BlitterCompletion::advance<AmigaHardware>();
    return 0;
}
#endif
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
#ifdef POP2_AUDIO
    NativeAudio::field();
#endif
    ++g_systemWindowFields;++g_scaffoldFields;timingSample();return 0; }
// Scheduling and exception-vector handback adapted from Alone in the Dark.
int32_t pop2SystemWindow(int32_t (*operation)(void*),void* context) {
    if(!operation || !vertbTaken || !g_coreServiceActive || g_systemWindowActive ||
       SysBase->TDNestCnt!=0 || SysBase->IDNestCnt!=-1)return -50;
    #ifdef POP2_DISPLAY
    display.quiesce();
    #endif
    Disable();
    if(!CoreRuntime::suspendVectors()) { Enable();return -50; }
    uint16_t mask=AmigaHardware::enabledInterrupts();
    IntVector portVertb=SysBase->IntVects[INTB_VERTB];
    #ifdef POP2_DISPLAY
    IntVector portBlit=SysBase->IntVects[INTB_BLIT];
    SysBase->IntVects[INTB_BLIT]=savedBlit;
    #endif
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
    #ifdef POP2_DISPLAY
    SysBase->IntVects[INTB_BLIT]=portBlit;
    AmigaHardware::clearInterruptRequests(INTF_BLIT);
    #endif
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
#ifdef POP2_AUDIO
    NativeAudio::field();
#endif
    ++g_scaffoldFields;timingSample();return 0; }
static_assert(__builtin_offsetof(ExecBase,IntVects)==84,"Exec vector layout changed");
bool PlatformAmiga::run() {
    g_scaffoldStage=1;
    GfxBase=(struct GfxBase*)OpenLibrary((CONST_STRPTR)"graphics.library",39);
    failure="SCAFFOLD / GRAPHICS LIBRARY";
    if(!GfxBase) { g_scaffoldError=1;return false; }
    failure="SCAFFOLD / DOS LIBRARY";
    DOSBase=(DosLibrary*)OpenLibrary((CONST_STRPTR)"dos.library",39);
    if(!DOSBase) { CloseLibrary((Library*)GfxBase);GfxBase=0;g_scaffoldError=2;return false; }
    failure=0;
    bool compatible=(SysBase->AttnFlags&AFF_68020) && (GfxBase->ChipRevBits0&GFXF_AA_LISA) && (GfxBase->DisplayFlags&PAL);
    if(!compatible) {
        failure="SCAFFOLD / REQUIRES PAL AGA 68020";
        PutStr((CONST_STRPTR)"SCAFFOLD / REQUIRES PAL AGA 68020\n");
        CloseLibrary((Library*)DOSBase);DOSBase=0;CloseLibrary((Library*)GfxBase);GfxBase=0;g_scaffoldError=3;return false;
    }
#ifdef POP2_CORE
    if(!CoreRuntime::prepare()) {
        failure=coreFailure();
        PutStr((CONST_STRPTR)CoreRuntime::error());PutStr((CONST_STRPTR)"\n");
        pop2CoreStopped();CoreRuntime::release();
        CloseLibrary((Library*)DOSBase);DOSBase=0;CloseLibrary((Library*)GfxBase);GfxBase=0;
        return false;
    }
#endif
#if defined(POP2_DISPLAY_PROBE) || defined(POP2_DISPLAY)
    if(!display.allocate()) {
        g_scaffoldError=5;failure="DISPLAY / ALLOCATE";
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
        failure=NativeInput::error();
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
#if defined(POP2_DISPLAY_PROBE) || defined(POP2_DISPLAY)
    savedBlit=SysBase->IntVects[INTB_BLIT];
    AmigaHardware::setInterrupts(INTF_BLIT,false);
    AmigaHardware::clearInterruptRequests(INTF_BLIT);
    blitServer.is_Node.ln_Type=NT_INTERRUPT;
    blitServer.is_Node.ln_Name=(char*)"Prince blitter";
    blitServer.is_Code=(void(*)())blitComplete;
    SysBase->IntVects[INTB_BLIT].iv_Data=0;
    SysBase->IntVects[INTB_BLIT].iv_Code=(void(*)())blitComplete;
    SysBase->IntVects[INTB_BLIT].iv_Node=&blitServer.is_Node;
#endif
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
        if(!display.publish(displayChunky,viewport,step==1?&dirty:nullptr,step==1?1:0,displayRGB,step==0)) {g_scaffoldError=6;break;}
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
#ifdef POP2_AUDIO
    NativeAudio::take();
#endif
    CoreRuntime::execute();
#ifdef POP2_AUDIO
    NativeAudio::restore();
#endif
#endif
#ifdef POP2_INPUT
    NativeInput::shutdown();
#endif
#if defined(POP2_DISPLAY_PROBE) || defined(POP2_DISPLAY)
    display.quiesce();
#endif
    AmigaHardware::setInterrupts(INTF_VERTB,false);
    AmigaHardware::clearInterruptRequests(INTF_VERTB);
    AmigaHardware::setCopperList(osCopper,true);
    *bplcon3Pointer=0x0c00;
    Disable();SysBase->IntVects[INTB_VERTB]=saved;
#if defined(POP2_DISPLAY_PROBE) || defined(POP2_DISPLAY)
    SysBase->IntVects[INTB_BLIT]=savedBlit;
#endif
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
#if defined(POP2_DISPLAY_PROBE) || defined(POP2_DISPLAY)
    const IntVector& restoredBlit=SysBase->IntVects[INTB_BLIT];
    g_vectorRestored=g_vectorRestored && restoredBlit.iv_Code==savedBlit.iv_Code &&
        restoredBlit.iv_Data==savedBlit.iv_Data && restoredBlit.iv_Node==savedBlit.iv_Node;
#endif
    bool ok=g_actualDMA==g_savedDMA && g_actualIRQ==g_savedIRQ && g_viewRestored && g_vectorRestored;
#ifdef POP2_INPUT
    ok=ok && g_inputVectorRestored==1;
#endif
    failure=0;
    if(!ok){g_scaffoldError=4;failure="SCAFFOLD / RESTORE FAILED";}
    PutStr((CONST_STRPTR)(ok ? "SCAFFOLD RESTORED\n" : "SCAFFOLD / RESTORE FAILED\n"));
#ifdef POP2_CORE
    PutStr((CONST_STRPTR)CoreRuntime::error());PutStr((CONST_STRPTR)"\n");
    if(!CoreRuntime::exitedNormally())failure=coreFailure();
    ok=ok && CoreRuntime::exitedNormally();
    g_scaffoldStage=3;pop2CoreStopped();CoreRuntime::release();
#endif
    CloseLibrary((Library*)DOSBase);DOSBase=0;
    CloseLibrary((Library*)GfxBase);GfxBase=0;
    g_scaffoldStage=3;
    pop2ScaffoldRestored();
#if defined(POP2_FAIL_EXIT)
    return false; // Negative control: the Shell must receive FAIL, not zero.
#else
    return ok;
#endif
}
