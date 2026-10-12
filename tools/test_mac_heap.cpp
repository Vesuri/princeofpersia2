#include <cstdint>
#include "../src/mac/MacHeap.h"
#include "../src/mac/RegionCopy.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <algorithm>
#define CHECK(x) do { if(!(x)){std::fprintf(stderr,"heap FAIL line %d: %s\n",__LINE__,#x);std::exit(1);} }while(0)
static void pattern(uint8_t* p,unsigned n,uint8_t v) {std::memset(p,v,n);}
static bool patternIs(uint8_t* p,unsigned n,uint8_t v) {for(unsigned i=0;i<n;++i)if(p[i]!=v)return false;return true;}
static void freeChain(MacHeap& heap,uint8_t* arena,std::vector<MacHeap::Handle> slots) {
    std::sort(slots.begin(),slots.end());
    MacHeap::Handle previous=nullptr;
    for(auto slot:slots)if(heap.isFreeHandleSlot(slot)) {
        CHECK(*slot==(uint8_t*)previous);previous=slot;
    }
    uint32_t head=0;for(unsigned i=8;i<12;++i)head=(head<<8)|arena[i];
    CHECK(head==uint32_t((unsigned long)previous));CHECK(heap.check());
}
static void checkRegionCopy() {
    alignas(8) uint8_t arena[16384];MacHeap h;CHECK(h.init(arena,sizeof(arena)));
    auto dst=h.newHandle(10,true);auto barrier=h.newPtr(128,true);
    auto src=h.newHandle(36,true);CHECK(dst && barrier && src);
    const int16_t words[]={36,1,2,4,8,1,2,4,6,8,32767,4,2,4,6,8,32767,32767};
    for(unsigned i=0;i<18;++i) {(*src)[2*i]=uint16_t(words[i])>>8;(*src)[2*i+1]=uint8_t(words[i]);}
    uint8_t expected[36];std::memcpy(expected,*src,36);
    CHECK(h.setState(src,0x40)==0 && h.setState(dst,0x40)==0);
    uint16_t size=0;CHECK(RegionCopy::copy(h,src,h,dst,size)==0 && size==36);
    CHECK(h.handleSize(dst)==36 && std::memcmp(*dst,expected,36)==0);
    CHECK(std::memcmp(*src,expected,36)==0 && h.state(src)==0x40 && h.state(dst)==0x40);
    CHECK(RegionCopy::copy(h,src,h,src,size)==0 && size==36);
    CHECK(std::memcmp(*src,expected,36)==0 && h.check());
    // Shrink a complex destination to a rectangular/empty region.
    (*src)[0]=0;(*src)[1]=10;std::memset(*src+2,0,8);
    CHECK(RegionCopy::copy(h,src,h,dst,size)==0 && h.handleSize(dst)==10);
    CHECK(std::memcmp(*src,*dst,10)==0);
    (*src)[1]=37;
    CHECK(RegionCopy::copy(h,src,h,dst,size)==MacHeap::paramErr && size==0);
    CHECK(h.handleSize(dst)==10 && (*dst)[1]==10 && h.state(src)==0x40);
    // A locked destination that cannot grow retains its contents and flags.
    auto locked=h.newHandle(10,true);auto obstruction=h.newPtr(128,true);
    CHECK(locked && obstruction);(*locked)[1]=10;CHECK(h.setState(locked,0x80)==0);
    std::memcpy(*src,expected,36);
    CHECK(RegionCopy::copy(h,src,h,locked,size)!=0);
    CHECK(h.handleSize(locked)==10 && (*locked)[1]==10 && h.state(locked)==0x80);
    CHECK(h.state(src)==0x40 && h.check());
    std::puts("PASS region-copy: complex resize shrink alias validation locked-failure handle-state");
}
// Direct handle validation: interior, stale and forged data pointers fail
// exactly as the block-chain walk did; master tables beyond the fixed list
// fall back to walking the chain.
static void checkDirectLookup() {
    std::vector<uint8_t> storage(1<<20);
    uint8_t* arena=storage.data()+(8-((unsigned long)storage.data()&7))%8;
    MacHeap h;CHECK(h.init(arena,(1<<20)-8,4));
    std::vector<MacHeap::Handle> live;
    for(unsigned i=0;i<200;++i) {
        auto x=h.newHandle(40+i*7);CHECK(x);live.push_back(x);
        CHECK(h.handleSize(x)==40+i*7);
    }
    CHECK(h.check()); // 200 handles with 4 per master block: list overflow and chain fallback
    auto victim=live[17];uint8_t* stale=*victim;
    CHECK(h.disposeHandle(victim)==0);
    // A fake master slot pointing at the old data must not validate it.
    MacHeap::Handle fake=(MacHeap::Handle)(*live[20]);*fake=stale;
    CHECK(h.handleSize(fake)==0);
    auto y=live[30];uint8_t* real=*y;
    *y=real+8;CHECK(h.handleSize(y)==0 && h.error()==MacHeap::memWZErr);
    *y=real+1;CHECK(h.handleSize(y)==0);
    // A forged header copied inside another block still lacks the back-link.
    uint8_t* inner=*live[40]+48;std::memcpy(inner,real-24,24);
    *y=real;CHECK(h.handleSize(y)==40+30*7);
    auto w=live[50];uint8_t* wreal=*w;*w=inner+24;CHECK(h.handleSize(w)==0);*w=wreal;
    for(unsigned i=0;i<200;++i)if(i!=17)CHECK(h.disposeHandle(live[i])==0);
    CHECK(h.check());
    MacHeap small;alignas(8) static uint8_t compactArena[65536];
    CHECK(small.init(compactArena,sizeof(compactArena),2));
    for(unsigned i=0;i<6;++i) {auto z=small.newHandle(64);CHECK(z && small.handleSize(z)==64);}
    CHECK(small.check());
}
int main() {
    alignas(8) uint8_t arena[65536];MacHeap h;
    CHECK(h.init(arena,sizeof(arena)));CHECK(h.check());
    auto original=h.freeBytes();
    auto a=h.newHandle(300,true);auto b=h.newHandle(500);auto pin=h.newPtr(200);
    CHECK(a&&b&&pin);CHECK(patternIs(*a,300,0));pattern(*a,300,0x23);pattern(*b,500,0x45);pattern(pin,200,0x67);
    CHECK(!h.isFreeHandleSlot(a));CHECK(!h.isFreeHandleSlot(nullptr));
    CHECK(!h.isFreeHandleSlot((MacHeap::Handle)pin));
    CHECK(!h.isFreeHandleSlot((MacHeap::Handle)((uint8_t*)a+1)));
    auto before=*b;CHECK(h.setState(b,0xa0)==0);CHECK(h.state(b)==0xa0);
    CHECK(h.disposeHandle(a)==0);CHECK(h.isFreeHandleSlot(a));CHECK(!h.isHandle(a));h.compact();CHECK(*b==before);CHECK(patternIs(*b,500,0x45));CHECK(patternIs(pin,200,0x67));
    CHECK(h.emptyHandle(b)==MacHeap::memPurErr);CHECK(h.moveHigh(b)==MacHeap::memLockedErr);
    CHECK(h.setState(b,0x20)==0);h.compact();CHECK(h.check());CHECK(h.recoverHandle(*b+100)==b);
    CHECK(h.moveHigh(b)==0);CHECK(h.check());CHECK(patternIs(*b,500,0x45));
    CHECK(h.setHandleSize(b,1500)==0);CHECK(h.handleSize(b)==1500);CHECK(patternIs(*b,500,0x45));
    CHECK(h.setHandleSize(b,24)==0);CHECK(patternIs(*b,24,0x45));CHECK(h.emptyHandle(b)==0);CHECK(*b==nullptr);
    CHECK(h.reallocateHandle(b,1000)==0);CHECK(h.state(b)==0);CHECK(h.handleSize(b)==1000);
    CHECK(h.disposeHandle(b)==0);CHECK(h.disposePtr(pin)==0);CHECK(h.freeBytes()==original);CHECK(h.check());
    // Failed reallocation retains the pointer, contents and state.
    a=h.newHandle(400,true);pattern(*a,400,0x37);before=*a;CHECK(h.setState(a,0x60)==0);
    CHECK(h.reallocateHandle(a,0xffffffff)==MacHeap::memFullErr);
    CHECK(*a==before);CHECK(patternIs(*a,400,0x37));CHECK(h.state(a)==0x60);
    CHECK(h.setState(a,0x80)==0);CHECK(h.reallocateHandle(a,1)==MacHeap::memPurErr);
    CHECK(h.disposeHandle(a)==0);
    // MoreMasters really allocates master blocks in the zone and reuses disposed slots.
    std::vector<MacHeap::Handle> handles;
    for(unsigned i=0;i<160;++i){auto x=h.newEmptyHandle();CHECK(x);CHECK(h.owns(x));handles.push_back(x);}
    CHECK(h.freeBytes()<original);for(auto x:handles)CHECK(h.disposeHandle(x)==0);CHECK(h.check());
    // Purging releases eligible data and preserves a stable empty master pointer.
    CHECK(h.init(arena,sizeof(arena)));
    a=h.newHandle(16000);b=h.newHandle(16000);CHECK(a&&b);pattern(*b,16000,0x91);
    CHECK(h.setState(a,0x40)==0);CHECK(h.setState(b,0xc0)==0);
    auto c=h.newHandle(40000);CHECK(!c);CHECK(*a==nullptr);CHECK(*b!=nullptr);CHECK(patternIs(*b,16000,0x91));CHECK(h.check());
    CHECK(h.setState(b,0)==0);c=h.newHandle(40000);CHECK(c);CHECK(patternIs(*b,16000,0x91));CHECK(h.check());
    CHECK(h.newHandle(0xffffffff)==nullptr);CHECK(h.error()==MacHeap::memFullErr);CHECK(h.check());
    // Pointer growth may move adjacent unlocked handles, never the pointer itself.
    CHECK(h.init(arena,sizeof(arena)));pin=h.newPtr(32);a=h.newHandle(512);pattern(*a,512,0x52);
    CHECK(h.setPtrSize(pin,8192)==0);CHECK(h.ptrSize(pin)==8192);CHECK(patternIs(*a,512,0x52));CHECK(h.check());
    CHECK(h.setState(a,0x80)==0);before=*a;CHECK(h.setHandleSize(a,128)==0);CHECK(*a==before);CHECK(patternIs(*a,128,0x52));
    CHECK(h.disposeHandle(a)==0);CHECK(h.disposePtr(pin)==0);
    CHECK(h.newHandle(0)!=nullptr);CHECK(h.newPtr(0)!=nullptr);CHECK(h.check());
    // A small region grows beside a large live allocation. Its master pointer,
    // flags and prefix survive relocation; locked neighbours and their data
    // remain valid. A later failed growth must also preserve that contract.
    CHECK(h.init(arena,sizeof(arena)));
    a=h.newHandle(252);b=h.newHandle(24000);auto barrier=h.newHandle(128);
    CHECK(a&&b&&barrier);pattern(*a,252,0x31);pattern(*b,24000,0x72);
    pattern(*barrier,128,0x93);CHECK(h.setState(a,0x60)==0);
    CHECK(h.setState(barrier,0x80)==0);auto fixedBarrier=*barrier;
    CHECK(h.setHandleSize(a,516)==0 && h.handleSize(a)==516 && h.state(a)==0x60);
    CHECK(patternIs(*a,252,0x31) && patternIs(*b,24000,0x72));
    CHECK(*barrier==fixedBarrier && patternIs(*barrier,128,0x93) && h.check());
    CHECK(h.setHandleSize(a,sizeof(arena))==MacHeap::memFullErr);
    CHECK(h.handleSize(a)==516 && h.state(a)==0x60 && patternIs(*a,252,0x31));
    CHECK(patternIs(*b,24000,0x72) && *barrier==fixedBarrier && h.check());
    // Read-only success and failure retain every arena byte, including the
    // published zone header and free-master chain, while reporting MemError.
    CHECK(h.init(arena,sizeof(arena),2));
    a=h.newHandle(37);b=h.newEmptyHandle();pin=h.newPtr(19);
    auto freed=h.newHandle(8);CHECK(h.disposeHandle(freed)==0);
    CHECK(h.setState(a,0xa0)==0);
    std::vector<uint8_t> queryArena(arena,arena+sizeof(arena));
    CHECK(h.handleSize(a)==37 && h.error()==0);
    CHECK(h.handleSize(b)==0 && h.error()==MacHeap::nilHandleErr);
    CHECK(h.handleSize(nullptr)==0 && h.error()==MacHeap::nilHandleErr);
    CHECK(h.ptrSize(pin)==19 && h.error()==0);
    CHECK(h.ptrSize(*a)==0 && h.error()==MacHeap::memWZErr);
    CHECK(h.state(a)==0xa0 && h.error()==0);
    CHECK(h.state(freed)==0 && h.error()==MacHeap::nilHandleErr);
    CHECK(h.recoverHandle(*a+5)==a && h.error()==0);
    CHECK(h.recoverHandle(pin)==nullptr && h.error()==MacHeap::nilHandleErr);
    CHECK(std::memcmp(arena,queryArena.data(),sizeof(arena))==0);
    CHECK(h.check());
    // Observe the published free-master chain across slot allocation/disposal,
    // failed allocation, and mutations which leave slot membership unchanged.
    CHECK(h.init(arena,sizeof(arena),2));
    std::vector<MacHeap::Handle> slots;
    for(unsigned i=0;i<6;++i){auto slot=h.newEmptyHandle();CHECK(slot);slots.push_back(slot);}
    freeChain(h,arena,slots);
    CHECK(h.disposeHandle(slots[1])==0);CHECK(h.disposeHandle(slots[4])==0);
    freeChain(h,arena,slots);
    CHECK(h.reallocateHandle(slots[0],100)==0);CHECK(h.setState(slots[0],0x60)==0);
    freeChain(h,arena,slots);
    CHECK(h.setHandleSize(slots[0],500)==0);CHECK(h.emptyHandle(slots[0])==0);
    h.compact();freeChain(h,arena,slots);
    CHECK(h.newHandle(0xffffffff)==nullptr);freeChain(h,arena,slots);
    a=h.newEmptyHandle();CHECK(a==slots[1]);freeChain(h,arena,slots);
    CHECK(h.disposeHandle(a)==0);freeChain(h,arena,slots);
    // A later master block may occupy a lower hole. Keep both the published
    // descending chain and lowest-address allocation order across that insert.
    CHECK(h.init(arena,sizeof(arena),2));
    a=h.newHandle(1024);b=h.newHandle(512);c=h.newEmptyHandle();
    CHECK(a&&b&&c);pattern(*b,512,0x71);
    CHECK(h.disposeHandle(a)==0);CHECK(h.moreMasters(2)==0);
    CHECK(h.newEmptyHandle()==a);
    auto low=h.newEmptyHandle(),lowNext=h.newEmptyHandle(),high=h.newEmptyHandle();
    CHECK(low<c && lowNext==low+1 && high==c+1);
    slots={a,b,c,low,lowNext,high};freeChain(h,arena,slots);
    CHECK(h.disposeHandle(low)==0);CHECK(h.disposeHandle(c)==0);
    h.compact();CHECK(h.moveHigh(b)==0);CHECK(patternIs(*b,512,0x71));
    freeChain(h,arena,slots);
    CHECK(h.newEmptyHandle()==low);CHECK(h.newEmptyHandle()==c);
    freeChain(h,arena,slots);
    // Match resource loading: target, free gap, movable data, locked barrier.
    // Include overlapping target/gap swaps as well as a large free interval.
    for(unsigned gapBytes: {8u,2048u,16000u}) {
        CHECK(h.init(arena,sizeof(arena)));
        a=h.newHandle(1024);auto hole=h.newHandle(gapBytes);b=h.newHandle(512);
        auto barrier=h.newHandle(128);CHECK(a&&b&&hole&&barrier);
        pattern(*a,1024,0x6d);pattern(*b,512,0x42);pattern(*barrier,128,0x91);
        CHECK(h.setState(barrier,0x80)==0);auto fixed=*barrier;before=*a;
        auto middle=*b;auto targetSpan=*hole-before;
        auto middleSpan=*barrier-middle;auto freeBefore=h.freeBytes();
        CHECK(h.disposeHandle(hole)==0);auto gapSpan=h.freeBytes()-freeBefore;
        auto freeAfter=h.freeBytes();CHECK(h.moveHigh(a)==0);
        CHECK(*a==before+gapSpan+middleSpan && *b==middle-targetSpan);
        CHECK(patternIs(*a,1024,0x6d) && patternIs(*b,512,0x42));
        CHECK(*barrier==fixed && patternIs(*barrier,128,0x91));
        CHECK(h.state(barrier)==0x80 && h.freeBytes()==freeAfter && h.check());
    }
    // Deterministic fragmentation: validate every surviving payload after every operation.
    CHECK(h.init(arena,sizeof(arena)));handles.assign(48,nullptr);uint32_t rng=1;
    unsigned sizes[48]={};
    for(unsigned step=0;step<2500;++step) {
        rng=rng*1664525U+1013904223U;unsigned slot=(rng>>16)%48;
        if(handles[slot]) { CHECK(h.disposeHandle(handles[slot])==0);handles[slot]=nullptr; }
        else { unsigned n=(rng%700)+1;auto x=h.newHandle(n);if(x){handles[slot]=x;sizes[slot]=n;pattern(*x,n,slot+1);} }
        if(step%7==0)h.compact();
        if(step%11==0 && handles[slot])CHECK(h.moveHigh(handles[slot])==0);
        CHECK(h.check());
        for(unsigned i=0;i<48;++i)if(handles[i])CHECK(patternIs(*handles[i],sizes[i],i+1));
    }
    // Force many master blocks interleaved with data and holes.
    CHECK(h.init(arena,sizeof(arena),1));handles.clear();
    for(unsigned i=0;i<120;++i){auto x=h.newHandle(200);CHECK(x);pattern(*x,200,i+1);handles.push_back(x);}
    for(unsigned i=0;i<120;i+=3){CHECK(h.disposeHandle(handles[i])==0);handles[i]=nullptr;}
    h.compact();CHECK(h.check());
    for(unsigned i=0;i<120;++i)if(handles[i]){CHECK(h.moveHigh(handles[i])==0);CHECK(h.setHandleSize(handles[i],300)==0);CHECK(patternIs(*handles[i],200,i+1));}
    h.compact();CHECK(h.check());
    for(unsigned i=0;i<120;++i)if(handles[i])CHECK(patternIs(*handles[i],200,i+1));
    checkRegionCopy();
    checkDirectLookup();
    std::puts("PASS mac-heap: direct-lookup allocation master-blocks lock purge compact resize move-high fragmentation=2500");
}
