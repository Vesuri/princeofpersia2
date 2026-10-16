#include <proto/exec.h>
#include <exec/memory.h>
#include <hardware/dmabits.h>
#include "PrinceScreen.h"
#include "mac/DirtyBands.h"
#include "mac/BlitterCompletion.h"
#include "framework/AmigaHardware.h"
extern "C" {
#ifdef POP2_TIMING_PROBE
extern volatile uint32_t g_timingPhase;
volatile uint32_t g_c2pPasses=0,g_c2pRects=0,g_c2pPixels=0,g_c2pUnbatched=0;
#endif
void pop2KalmsC2PRect(const uint8_t*,uint8_t*,uint32_t,uint32_t);
volatile uint32_t g_displayPlanes=0,g_displayCopper=0,g_displayFields=0,g_displayFrames=0;
volatile uint16_t g_displayParity=0,g_displayLine=0,g_displayLate=0,g_displayLOF=0;
__attribute__((noinline)) void pop2DisplayPublished() { __asm__ volatile("nop" ::: "memory"); }
}
// DERIVED from 512 hires pixels and 192 lines per PAL field, centred as Vette.
// FMODE 3 fetches eight 64-pixel groups, 16 colour clocks apart. The first
// fetch is four clocks earlier than the 16-bit hires window. The emulator
// gate decodes this mode and both fields; real hardware remains a release gate.
static const uint16_t mode[][2]={
 {0x1fc,3},{0x100,0x8215},{0x102,0},{0x104,0x24},{0x106,AgaPalette::control},
 {0x10c,0},{0x08e,0x4ca1},{0x090,0x0ca1},{0x1e4,0x2100},
 {0x092,0x0048},{0x094,0x00b8},{0x108,960},{0x10a,960}
};
static_assert(2*Planar8::rowBytes-Planar8::planeRow==960,"interlaced modulo");
bool PrinceScreen::allocate() {
    for(unsigned i=0;i<2;++i) {
        pixels_[i]=(uint8_t*)AllocMem(Planar8::bytes,MEMF_CHIP|MEMF_CLEAR);
        if(!pixels_[i] || ((uint32_t)pixels_[i]&7)) {release();return false;}
    }
    copper_=(uint32_t*)AllocMem(4*copperMoves*4,MEMF_CHIP|MEMF_CLEAR);
    if(!copper_) {release();return false;}
    uint32_t black[256]={};lists(0,black);lists(1,black);return true;
}
void PrinceScreen::lists(uint16_t buffer,const uint32_t* rgb) {
    for(unsigned parity=0;parity<2;++parity) {
        uint32_t* c=copper_+(buffer*2+parity)*copperMoves;unsigned at=0;
        for(unsigned plane=0;plane<8;++plane) {
            uint32_t address=(uint32_t)pixels_[buffer]+parity*Planar8::rowBytes+plane*Planar8::planeRow;
            c[at++]=AgaPalette::move(0xe0+4*plane,address>>16);
            c[at++]=AgaPalette::move(0xe2+4*plane,address);
        }
        for(unsigned i=0;i<13;++i)c[at++]=AgaPalette::move(mode[i][0],mode[i][1]);
        AgaPalette::build(c+at,rgb);at+=AgaPalette::moves;c[at]=0xfffffffe;
    }
}
void PrinceScreen::start() {
    if(!copper_)return;
    // Copper DMA is disabled by the takeover. Establish LACE before sampling it.
    for(unsigned i=0;i<13;++i)*(volatile uint16_t*)(0xdff000+mode[i][0])=mode[i][1];
    started_=true;field();
    AmigaHardware::setDMAChannels(DMAF_MASTER|DMAF_COPPER|DMAF_RASTER,true);
}
void PrinceScreen::field() {
    if(!started_)return;
    g_displayLOF=(*vposrPointer&0x8000) ? 1 : 0;
    const uint16_t parity=g_displayLOF ? 0 : 1;
    // Change pictures on the even field so both halves share pixels and colours.
    bool changed=pending_ && !parity;
    if(changed)front_^=1;
    uint32_t* list=copper_+(front_*2+parity)*copperMoves;
    *cop1lcPointer=list;*copjmp1Pointer=0;
    uint16_t high=*vposrPointer,low=*vhposrPointer;
    g_displayLine=((high&1)<<8)|(low>>8);
    if(g_displayLine>=16)++g_displayLate;
    g_displayPlanes=(uint32_t)pixels_[front_];g_displayCopper=(uint32_t)list;
    g_displayParity=parity;++g_displayFields;
    if(changed) {
        // The old front is now inactive. Copy this completed picture's bands
        // while the game works in Fast RAM. Level-3 completion cannot interrupt
        // this level-3 producer; pending_ stays set until the batch is armed.
        // At most 64 * ceil(384/127) * 25 queue words, below the shared capacity.
        static_assert(64*4*25 < BLITTER_QUEUE_SIZE,"display blit batch capacity");
        for(unsigned i=0;i<syncCount_;++i)synchronize(sync_[i],front_^1);
        if(syncCount_)BlitterCompletion::advance<AmigaHardware>();
        ++g_displayFrames;pending_=0;pop2DisplayPublished();
    }
}
bool PrinceScreen::publish(const uint8_t* chunky,const Planar8::Rect& viewport,
                           const Planar8::Rect* dirty,uint16_t count,const uint32_t* rgb,bool prepareScene) {
    if(!started_ || !chunky || !rgb || !Planar8::viewportValid(viewport) || (count && !dirty))return false;
    while(pending_) {__asm__ volatile("nop" ::: "memory");}
    Planar8::Rect work[64];uint16_t n=0;
    bool moved=!valid_ || viewport.top!=viewport_.top || viewport.left!=viewport_.left;
    // Union clipped coordinates before alignment, as in Alone in the Dark.
    // Disjoint horizontal bands avoid converting actor overlaps repeatedly.
    if(!DirtyBands::plan<Planar8::width,Planar8::height,64>(viewport,dirty,count,work,64,n))return false;
    if(moved) {work[0]={0,0,Planar8::height,Planar8::width};n=1;}
    uint16_t back=front_^1;
    // Completion normally ran during game work. Never race a late copy with
    // the next C2P write (including viewport or palette-only publication).
    BlitterCompletion::drain<AmigaHardware>();
#ifdef POP2_TIMING_PROBE
    if(n){++g_c2pPasses;g_c2pRects+=n;if(g_timingPhase!=0x1002)++g_c2pUnbatched;}
#endif
    for(unsigned i=0;i<n;++i) {
        auto r=work[i];
#ifdef POP2_TIMING_PROBE
        g_c2pPixels+=uint32_t(r.right-r.left)*uint32_t(r.bottom-r.top);
#endif
        pop2KalmsC2PRect(chunky+uint32_t(viewport.top+r.top)*640+viewport.left+r.left,
            pixels_[back]+uint32_t(r.top)*Planar8::rowBytes+r.left/8,r.right-r.left,r.bottom-r.top);
        sync_[i]=r;
    }
    syncCount_=n;viewport_=viewport;valid_=true;lists(back,rgb);
    __asm__ volatile("" ::: "memory");pending_=1;
    uint32_t changedPixels=0;
    for(unsigned i=0;i<n;++i)changedPixels+=uint32_t(work[i].right-work[i].left)*(work[i].bottom-work[i].top);
    if(prepareScene && changedPixels>uint32_t(Planar8::width)*Planar8::height/2) {
        // Finish a large scene replacement in both buffers before returning to
        // the game. Deferring this copy would charge the following small update
        // for the scene load. Copy only after VBI makes the old front inactive.
        quiesce();
    }
    return true;
}
void PrinceScreen::synchronize(const Planar8::Rect& r,uint16_t back) {
    // Alone in the Dark's synchronization path, using the shared blitter.
    const uint16_t widthBytes=uint16_t(r.right-r.left)/8;
    AmigaHardware::setDMAChannels(DMAF_BLITTER,true);
    for(int16_t y=r.top;y<r.bottom;) {
        const uint16_t lines=r.bottom-y>127?127:r.bottom-y;
        const uint32_t offset=uint32_t(y)*Planar8::rowBytes+r.left/8;
        AmigaHardware::blitterCopy((uint16_t*)(pixels_[front_]+offset),
            (uint16_t*)(pixels_[back]+offset),widthBytes/2,lines*8,
            Planar8::planeRow-widthBytes,Planar8::planeRow-widthBytes,
            0,0xffff,0xffff,0xffff);
        y+=lines;
    }
}
void PrinceScreen::quiesce() {
    wait();
    BlitterCompletion::drain<AmigaHardware>();
}
// Same completed-frame rule as Alone in the Dark: publication must finish,
// and an unchanged scene must still cross a vertical blank before reuse.
void PrinceScreen::paceFrame() {
    extern volatile uint32_t g_scaffoldFields;
    while(pending_ || g_scaffoldFields==lastFrameField_) {__asm__ volatile("nop" ::: "memory");}
    lastFrameField_=g_scaffoldFields;
}
void PrinceScreen::release() {
    started_=false;
    if(copper_)FreeMem(copper_,4*copperMoves*4);copper_=nullptr;
    for(unsigned i=0;i<2;++i) {if(pixels_[i])FreeMem(pixels_[i],Planar8::bytes);pixels_[i]=nullptr;}
}
