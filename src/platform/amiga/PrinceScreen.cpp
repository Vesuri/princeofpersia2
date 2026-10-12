#include <proto/exec.h>
#include <exec/memory.h>
#include <hardware/dmabits.h>
#include "PrinceScreen.h"
#include "framework/AmigaHardware.h"
extern "C" {
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
    if(changed) {++g_displayFrames;pending_=0;pop2DisplayPublished();}
}
bool PrinceScreen::publish(const uint8_t* chunky,const Planar8::Rect& viewport,
                           const Planar8::Rect* dirty,uint16_t count,const uint32_t* rgb) {
    if(!started_ || !chunky || !rgb || !Planar8::viewportValid(viewport) || (count && !dirty))return false;
    while(pending_) {__asm__ volatile("nop" ::: "memory");}
    Planar8::Rect work[64];uint16_t n=0;
    bool moved=!valid_ || viewport.top!=viewport_.top || viewport.left!=viewport_.left;
    if(moved)work[n++]={0,0,Planar8::height,Planar8::width};
    for(unsigned i=0;i<count;++i) {
        Planar8::Rect r;if(!Planar8::normalize(viewport,dirty[i],r))return false;
        if(!moved && r.top<r.bottom && r.left<r.right)Planar8::append(work,n,64,r);
    }
    uint16_t back=front_^1;
    for(unsigned i=0;i<syncCount_;++i)if(Planar8::syncNeeded(sync_[i],work,n)) {
        auto r=sync_[i];
        for(int y=r.top;y<r.bottom;++y)for(unsigned plane=0;plane<8;++plane)
            for(int x=r.left/8;x<r.right/8;++x) {
                uint32_t at=uint32_t(y)*Planar8::rowBytes+plane*Planar8::planeRow+x;
                pixels_[back][at]=pixels_[front_][at];
            }
    }
    for(unsigned i=0;i<n;++i) {
        auto r=work[i];
        pop2KalmsC2PRect(chunky+uint32_t(viewport.top+r.top)*640+viewport.left+r.left,
            pixels_[back]+uint32_t(r.top)*Planar8::rowBytes+r.left/8,r.right-r.left,r.bottom-r.top);
        sync_[i]=r;
    }
    syncCount_=n;viewport_=viewport;valid_=true;lists(back,rgb);
    __asm__ volatile("" ::: "memory");pending_=1;return true;
}
void PrinceScreen::release() {
    started_=false;
    if(copper_)FreeMem(copper_,4*copperMoves*4);copper_=nullptr;
    for(unsigned i=0;i<2;++i) {if(pixels_[i])FreeMem(pixels_[i],Planar8::bytes);pixels_[i]=nullptr;}
}
