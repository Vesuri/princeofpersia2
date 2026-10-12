// Takeover/handback order adapted from Alone in the Dark; no game services.
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/dos.h>
#include <exec/execbase.h>
#include <exec/interrupts.h>
#include <graphics/gfxbase.h>
#include <hardware/dmabits.h>
#include <hardware/intbits.h>
#include "PlatformAmiga.h"
#include "framework/CopperList.h"
#include "framework/AmigaHardware.h"
extern DosLibrary* DOSBase;
extern "C" {
volatile uint32_t g_scaffoldStage=0, g_scaffoldFields=0, g_scaffoldError=0;
volatile uint16_t g_savedDMA=0,g_actualDMA=0,g_savedIRQ=0,g_actualIRQ=0,g_viewRestored=0,g_vectorRestored=0;
__attribute__((noinline)) void pop2ScaffoldRestored() { __asm__ volatile("nop" ::: "memory"); }
}
static Interrupt server;
static uint32_t vbi() { *intreqPointer=INTF_VERTB;++g_scaffoldFields;return 0; }
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
    View* view=GfxBase->ActiView;
    CopperList osCopper((uint32_t*)GfxBase->copinit);
    const uint16_t dmaMask=DMAF_ALL|DMAF_MASTER|DMAF_BLITHOG;
    g_savedDMA=AmigaHardware::enabledDMAChannels()&dmaMask;
    g_savedIRQ=AmigaHardware::enabledInterrupts();
    LoadView(0);WaitTOF();WaitTOF();
    AmigaHardware::setDMAChannels(DMAF_RASTER|DMAF_SPRITE|DMAF_COPPER,false);
    *bplcon0Pointer=0;
    server.is_Node.ln_Type=NT_INTERRUPT;server.is_Node.ln_Name=(char*)"Prince scaffold VBI";
    server.is_Code=(void(*)())vbi;
    Disable();IntVector saved=SysBase->IntVects[INTB_VERTB];
    SysBase->IntVects[INTB_VERTB].iv_Data=0;
    SysBase->IntVects[INTB_VERTB].iv_Code=(void(*)())vbi;
    SysBase->IntVects[INTB_VERTB].iv_Node=&server.is_Node;
    Enable();
    Forbid();
    g_scaffoldStage=2;
    AmigaHardware::setInterrupts(INTF_INTEN|INTF_VERTB,true);
    while(g_scaffoldFields<4) { __asm__ volatile("nop"); }
    AmigaHardware::setInterrupts(INTF_VERTB,false);
    AmigaHardware::clearInterruptRequests(INTF_VERTB);
    AmigaHardware::setCopperList(osCopper,true);
    *bplcon3Pointer=0x0c00;
    Disable();SysBase->IntVects[INTB_VERTB]=saved;Enable();
    AmigaHardware::setDMAChannels(dmaMask,false);
    AmigaHardware::setInterrupts(0x7fff,false);
    AmigaHardware::setDMAChannels(g_savedDMA,true);
    AmigaHardware::setInterrupts(g_savedIRQ,true);
    Permit();LoadView(view);WaitTOF();WaitTOF();
    g_actualDMA=AmigaHardware::enabledDMAChannels()&dmaMask;
    g_actualIRQ=AmigaHardware::enabledInterrupts();
    g_viewRestored=GfxBase->ActiView==view;
    IntVector& restored=SysBase->IntVects[INTB_VERTB];
    g_vectorRestored=restored.iv_Code==saved.iv_Code && restored.iv_Data==saved.iv_Data && restored.iv_Node==saved.iv_Node;
    bool ok=g_actualDMA==g_savedDMA && g_actualIRQ==g_savedIRQ && g_viewRestored && g_vectorRestored;
    if(!ok)g_scaffoldError=4;
    PutStr((CONST_STRPTR)(ok ? "SCAFFOLD RESTORED\n" : "SCAFFOLD / RESTORE FAILED\n"));
    CloseLibrary((Library*)DOSBase);DOSBase=0;
    CloseLibrary((Library*)GfxBase);GfxBase=0;
    g_scaffoldStage=3;
    pop2ScaffoldRestored();
#ifdef POP2_FAIL_EXIT
    return false; // Negative control: the Shell must receive FAIL, not zero.
#else
    return ok;
#endif
}
