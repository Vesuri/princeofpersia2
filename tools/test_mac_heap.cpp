#include <cstdint>
#include "../src/mac/MacHeap.h"
#include "../src/mac/RegionCopy.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <algorithm>
#include <string>
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
static void checkCoalesced(const uint8_t* arena,uint32_t bytes) {
    bool previousFree=false;
    for(uint32_t at=64;at<bytes-16;) {
        uint32_t span,kind;std::memcpy(&span,arena+at,4);std::memcpy(&kind,arena+at+12,4);
        CHECK(span>=32 && at+span<=bytes-16);
        CHECK(!(previousFree && kind==0));previousFree=kind==0;at+=span;
    }
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
static void checkLockHandle() {
    alignas(8) uint8_t arena[8192],before[8192],expected[8192];
    for(unsigned state=0;state<256;++state) {
        MacHeap h;CHECK(h.init(arena,sizeof(arena),4));
        auto handle=h.newHandle(37,true);CHECK(handle);
        CHECK(h.setState(handle,state)==0);pattern(*handle,37,0x5a);
        MacHeap saved=h;std::memcpy(before,arena,sizeof(arena));
        CHECK(h.setState(handle,h.state(handle)|0x80)==0);
        std::memcpy(expected,arena,sizeof(arena));
        h=saved;std::memcpy(arena,before,sizeof(arena));
        CHECK(h.lockHandle(handle)==0 && h.error()==0);
        CHECK(!std::memcmp(arena,expected,sizeof(arena)) && h.check());
        CHECK(h.state(handle)==((state&0xe0)|0x80));
        CHECK(h.lockHandle(handle)==0 && !std::memcmp(arena,expected,sizeof(arena)));
        CHECK(h.disposeHandle(handle)==0);
        std::memcpy(before,arena,sizeof(arena));
        CHECK(h.lockHandle(handle)==MacHeap::nilHandleErr);
        CHECK(h.lockHandle(nullptr)==MacHeap::nilHandleErr);
        CHECK(h.lockHandle((MacHeap::Handle)(arena+1))==MacHeap::nilHandleErr);
        CHECK(!std::memcmp(arena,before,sizeof(arena)) && h.check());
    }
    std::puts("PASS HLock: 256 state inputs, entire arena/publication equality, repeat/disposed/null/interior guards");
}
static void checkReadState() {
    alignas(8) uint8_t arena[32768],before[32768];
    MacHeap h;CHECK(h.init(arena,sizeof(arena),2));
    MacHeap::Handle handles[40];
    for(auto& handle:handles){handle=h.newHandle(11,true);CHECK(handle);}
    for(unsigned state=0;state<256;++state){
        auto handle=handles[state%40];CHECK(h.setState(handle,state)==0);
        std::memcpy(before,arena,sizeof(arena));
        CHECK(h.state(handle)==(state&0xe0) && h.error()==0);
        CHECK(!std::memcmp(arena,before,sizeof(arena)));
        CHECK(h.state((MacHeap::Handle)((uint8_t*)handle+1))==0 && h.error()==MacHeap::nilHandleErr);
        CHECK(h.state((MacHeap::Handle)*handle)==0 && h.error()==MacHeap::nilHandleErr);
        CHECK(h.state(nullptr)==0 && h.error()==MacHeap::nilHandleErr);
        CHECK(!std::memcmp(arena,before,sizeof(arena)));
    }
    for(auto handle:handles){
        CHECK(h.disposeHandle(handle)==0);std::memcpy(before,arena,sizeof(arena));
        CHECK(h.state(handle)==0 && h.error()==MacHeap::nilHandleErr);
        CHECK(!std::memcmp(arena,before,sizeof(arena)) && h.check());
    }
    CHECK(h.init(arena,sizeof(arena),1));
    CHECK(!h.isFreeHandleSlot(handles[1]) && !h.isHandle(handles[1]));
    CHECK(h.state(handles[1])==0 && h.error()==MacHeap::nilHandleErr);
    std::puts("PASS HGetState: all state bytes, master-cache misses, read-only arena, invalid and disposed handles");
}
// MEASURED on System 7.5.5 (ref masters-3, tools/check_masters.py): after
// every free master is used, a new master block (implicit through NewHandle or
// explicit through MoreMasters) or a NewPtr block goes to the lowest position
// above pinned blocks, and an implicit handle's data follows it directly.
struct MasterCase { const char* layout; char kind; const char* below; };
static void checkMasterPlacement() {
    const MasterCase cases[]={{"hhhh",'i',""},{"hgLhh",'i',""},{"hgLhh",'e',""},
                              {"hgLhh",'p',""},{"Lghh",'i',"L1"},{"hgLLhgh",'i',""},
                              {"hLLh",'i',""},{"hLLh",'p',""}};
    for(const auto& c:cases) {
        alignas(8) static uint8_t arena[65536];MacHeap h;CHECK(h.init(arena,sizeof(arena),8));
        struct Item { std::string label;MacHeap::Handle handle; };
        std::vector<Item> items;std::vector<MacHeap::Handle> holes;
        for(const char* l=c.layout;*l;++l) {
            auto handle=h.newHandle(2048);CHECK(handle);
            if(*l=='g')holes.push_back(handle);
            else {
                if(*l=='L')CHECK(h.setState(handle,0x80)==0);
                items.push_back({std::string(1,*l)+std::to_string(items.size()+1),handle});
            }
        }
        for(auto hole:holes)CHECK(h.disposeHandle(hole)==0);
        for(size_t used=items.size();used<8;++used)CHECK(h.newEmptyHandle());
        uint8_t* block=nullptr;
        if(c.kind=='i') {
            auto handle=h.newHandle(512);CHECK(handle);block=(uint8_t*)handle;items.push_back({"d",handle});
        } else if(c.kind=='e') {
            CHECK(h.moreMasters()==0);auto handle=h.newEmptyHandle();CHECK(handle);block=(uint8_t*)handle;
        } else {block=h.newPtr(256);CHECK(block);}
        std::vector<std::pair<uint8_t*,std::string>> order{{block,"M"}};
        for(auto& item:items)order.push_back({*item.handle,item.label});
        std::sort(order.begin(),order.end());
        std::string below;size_t at=0;
        while(order[at].second!="M"){below+=order[at].second;++at;}
        CHECK(below==c.below);
        if(c.kind=='i')CHECK(order[at+1].second=="d");
        CHECK(h.check());
    }
}
// MEASURED (ref masters-3): an 11 KB request fits only by moving the low 8 KB
// handle across a locked one into a 9 KB hole; a 13 KB request fails.
static void checkRelocateAcrossLocked() {
    for(uint32_t request: {11264u,13312u}) {
        alignas(8) static uint8_t arena[65536];MacHeap h;CHECK(h.init(arena,sizeof(arena),8));
        auto h1=h.newHandle(8192),g=h.newHandle(4096),l2=h.newHandle(1024);
        CHECK(h1 && g && l2 && h.setState(l2,0x80)==0);pattern(*h1,8192,0x5a);
        auto f=h.newHandle(9216),l3=h.newHandle(1024);CHECK(f && l3 && h.setState(l3,0x80)==0);
        auto rest=h.newHandle(h.largestBlock());CHECK(rest && h.setState(rest,0x80)==0);
        CHECK(h.disposeHandle(g)==0 && h.disposeHandle(f)==0);
        auto before=*h1;auto lockedA=*l2,lockedB=*l3;
        auto handle=h.newHandle(request);
        if(request==11264) {
            CHECK(handle && *handle<before+1 && *h1>*l2 && *h1<*l3 && patternIs(*h1,8192,0x5a));
        } else CHECK(!handle && h.error()==MacHeap::memFullErr && *h1==before);
        CHECK(*l2==lockedA && *l3==lockedB && h.check());
    }
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
    CHECK(h.disposeHandle(nullptr)==0);CHECK(h.error()==0);CHECK(h.freeBytes()==original);CHECK(h.check());
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
    // The locked handle keeps the first new master block above the hole.
    CHECK(h.init(arena,sizeof(arena),2));
    a=h.newHandle(1024);CHECK(a && h.setState(a,0x80)==0);b=h.newHandle(512);c=h.newEmptyHandle();
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
        auto middle=*b;
        auto middleSpan=*barrier-middle;auto freeBefore=h.freeBytes();
        CHECK(h.disposeHandle(hole)==0);auto gapSpan=h.freeBytes()-freeBefore;
        auto freeAfter=h.freeBytes();CHECK(h.moveHigh(a)==0);
        CHECK(*a==before+gapSpan+middleSpan && *b==before);
        CHECK(patternIs(*a,1024,0x6d) && patternIs(*b,512,0x42));
        CHECK(*barrier==fixed && patternIs(*barrier,128,0x91));
        CHECK(h.state(barrier)==0x80 && h.freeBytes()==freeAfter && h.check());
    }
    // Original MoveHHi with a free tail: only the target moves. Locked
    // barriers bound the search; purgeable neighbours are retained.
    for(unsigned state: {0u,0x40u}) {
        CHECK(h.init(arena,sizeof(arena)));
        a=h.newHandle(1024);b=h.newHandle(512);auto hole=h.newHandle(8192);
        auto barrier=h.newHandle(512);CHECK(a&&b&&hole&&barrier);
        CHECK(h.setState(b,state)==0 && h.setState(barrier,0x80)==0);
        pattern(*a,1024,0x17);pattern(*b,512,0x31);pattern(*barrier,512,0x51);
        auto oldA=*a,oldB=*b,oldBarrier=*barrier;auto targetSpan=*b-*a;
        CHECK(h.disposeHandle(hole)==0);auto free=h.freeBytes();
        CHECK(h.moveHigh(a)==0 && *a==oldBarrier-targetSpan && *a>oldA);
        CHECK(*b==oldB && *barrier==oldBarrier && h.freeBytes()==free);
        CHECK(patternIs(*a,1024,0x17) && patternIs(*b,512,0x31));
        CHECK(patternIs(*barrier,512,0x51) && h.state(b)==state && h.check());
    }
    // Original no-room case still succeeds by moving the unlocked neighbour;
    // a purgeable neighbour is moved, not purged. A locked one blocks the move.
    for(unsigned state: {0u,0x40u,0x80u}) {
        CHECK(h.init(arena,sizeof(arena)));
        a=h.newHandle(1024);b=h.newHandle(512);
        auto rest=h.newHandle(h.largestBlock());CHECK(a&&b&&rest);
        CHECK(h.setState(rest,0x80)==0 && h.setState(b,state)==0);
        auto oldA=*a,oldB=*b;pattern(*a,1024,0x17);pattern(*b,512,0x31);
        CHECK(h.freeBytes()==0 && h.moveHigh(a)==0 && h.freeBytes()==0);
        if(state==0x80)CHECK(*a==oldA && *b==oldB);
        else CHECK(*a>oldA && *b==oldA);
        CHECK(h.state(b)==state && patternIs(*a,1024,0x17));
        CHECK(patternIs(*b,512,0x31) && h.check());
    }
    // Exact fit and a tail remainder too small for a free-block header.
    for(unsigned extra: {0u,8u,16u,24u,32u}) {
        CHECK(h.init(arena,sizeof(arena)));
        a=h.newHandle(1024);b=h.newHandle(512);auto hole=h.newHandle(1024+extra);
        auto barrier=h.newHandle(512);CHECK(a&&b&&hole&&barrier);
        CHECK(h.setState(barrier,0x80)==0);auto oldB=*b;
        pattern(*a,1024,0x17);CHECK(h.disposeHandle(hole)==0);
        CHECK(h.moveHigh(a)==0 && *b==oldB && h.handleSize(a)==1024);
        CHECK(patternIs(*a,1024,0x17) && h.check());
        CHECK(h.disposeHandle(a)==0 && h.disposeHandle(b)==0 && h.check());
    }
    // Paired with mac_heap.lua's System 7.5.5 in-place resize/failure cases.
    CHECK(h.init(arena,sizeof(arena)));
    a=h.newHandle(1024);b=h.newHandle(512);c=h.newHandle(128);
    CHECK(a&&b&&c && h.setState(c,0x80)==0);pattern(*a,1024,17);
    before=*a;CHECK(h.disposeHandle(b)==0);
    for(unsigned size: {1024u,256u,512u,1024u}) {
        CHECK(h.setHandleSize(a,size)==0 && *a==before && h.handleSize(a)==size);
        CHECK(patternIs(*a,256,17) && h.check());checkCoalesced(arena,sizeof(arena));
    }
    CHECK(h.setState(a,0x80)==0 && h.setHandleSize(a,131072)==MacHeap::memFullErr);
    CHECK(*a==before && h.handleSize(a)==1024 && patternIs(*a,256,17) && h.check());
    // Deterministic fragmentation: validate every surviving payload after every operation.
    CHECK(h.init(arena,sizeof(arena)));handles.assign(48,nullptr);uint32_t rng=1;
    unsigned sizes[48]={};
    for(unsigned step=0;step<2500;++step) {
        rng=rng*1664525U+1013904223U;unsigned slot=(rng>>16)%48;
        if(handles[slot] && step%3) {
            unsigned n=(rng%900)+1,old=sizes[slot];
            uint8_t state=h.state(handles[slot]);
            int error=h.setHandleSize(handles[slot],n);
            CHECK(error==0 || error==MacHeap::memFullErr);
            CHECK(h.state(handles[slot])==state);
            CHECK(patternIs(*handles[slot],error ? old : std::min(old,n),slot+1));
            if(!error) {sizes[slot]=n;pattern(*handles[slot],n,slot+1);}
        }
        else if(handles[slot]) { CHECK(h.disposeHandle(handles[slot])==0);handles[slot]=nullptr; }
        else { unsigned n=(rng%700)+1;auto x=h.newHandle(n);if(x){handles[slot]=x;sizes[slot]=n;pattern(*x,n,slot+1);} }
        if(step%7==0)h.compact();
        if(step%11==0 && handles[slot])CHECK(h.moveHigh(handles[slot])==0);
        CHECK(h.check());checkCoalesced(arena,sizeof(arena));
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
    checkLockHandle();
    checkReadState();checkMasterPlacement();checkRelocateAcrossLocked();
    checkRegionCopy();
    checkDirectLookup();
    std::puts("PASS mac-heap: direct-lookup allocation master-blocks lock purge compact resize move-high fragmentation=2500");
}
