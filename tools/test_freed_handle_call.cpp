#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <vector>
#include "../src/mac/MacHeap.h"
#include "../src/mac/FreedHandleCall.h"
#define CHECK(x) do { if(!(x)){std::fprintf(stderr,"freed call FAIL line %d\n",__LINE__);std::exit(1);} }while(0)
int main() {
    alignas(8) uint8_t arena[65536];MacHeap heap;
    unsigned cases=0;
    for(unsigned count: {1u,2u,3u,8u,32u,64u,65u})for(bool reverse: {false,true}) {
        CHECK(heap.init(arena,sizeof(arena),64));
        std::vector<MacHeap::Handle> handles;
        for(unsigned i=0;i<count;++i){auto h=heap.newHandle(16);CHECK(h);handles.push_back(h);}
        for(unsigned i=0;i<count;++i)CHECK(heap.disposeHandle(handles[reverse?count-1-i:i])==0);
        std::vector<uint8_t> before(arena,arena+sizeof(arena));
        for(auto h:handles) {
            CHECK(heap.isFreeHandleSlot(h) && !heap.isHandle(h));
            for(auto trap: {0xa02au,0xa049u})for(unsigned segment: {3u,15u,0u,23u}) {
                for(unsigned offset: {0x29aau,0x29c2u,0x29dau,0x5f9au,0x63aau,0x63acu,0x29b0u,0x29c8u,0x29e0u,0x29abu,0x29c4u,0u}) {
                    bool expected=(trap==0xa049 && segment==3 && (offset==0x5f9a || offset==0x63aa)) ||
                        (trap==0xa049 && segment==15 && (offset==0x29b0 || offset==0x29c8 || offset==0x29e0)) ||
                        (trap==0xa02a && segment==15 && (offset==0x29aa || offset==0x29c2 || offset==0x29da));
                    CHECK(FreedHandleCall::supported(trap,segment,offset)==expected);++cases;
                }
            }
            CHECK(!heap.isFreeHandleSlot((MacHeap::Handle)((uint8_t*)h+1)));
        }
        CHECK(!heap.isFreeHandleSlot(nullptr));
        CHECK(!heap.isFreeHandleSlot((MacHeap::Handle)arena));
        CHECK(!heap.isFreeHandleSlot((MacHeap::Handle)before.data()));
        CHECK(!heap.isFreeHandleSlot((MacHeap::Handle)(arena+sizeof(arena))));
        CHECK(std::memcmp(arena,before.data(),sizeof(arena))==0 && heap.check());
        auto live=heap.newHandle(16);CHECK(live && heap.isHandle(live) && !heap.isFreeHandleSlot(live));
    }
    for(unsigned trap: {0xa029u,0xa024u,0xa04au,0xa023u})
        CHECK(!FreedHandleCall::supported(trap,15,0x29c2));
    std::printf("FREED POLICY PASS cases=%u heap-preserved=1 unknown-rejected=1 reused-live=1\n",cases);
}
