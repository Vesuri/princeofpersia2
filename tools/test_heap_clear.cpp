#include <cstdint>
#include <cstring>
#include <cassert>
#include <cstdio>
#include "mac/MacHeap.h"
// Clear exactly the logical extent, including empty blocks and every longword
// tail. Reuse dirty payloads so a skipped clear cannot pass on fresh memory.
int main() {
    alignas(8) static uint8_t arena[65536];
    for(unsigned n=0;n<=16385;n=n<80?n+1:n+509) {
        MacHeap heap;std::memset(arena,0xa5,sizeof arena);
        assert(heap.init(arena,sizeof arena));
        auto h=heap.newHandle(n,false);assert(h);
        auto data=*h;std::memset(data,0x91,n);
        assert(heap.disposeHandle(h)==0);
        auto cleared=heap.newHandle(n,true);assert(cleared==h && *cleared==data);
        for(unsigned i=0;i<n;++i)assert(data[i]==0);
        assert(heap.handleSize(cleared)==n && heap.state(cleared)==0 && heap.check());
        // Physical payload padding belongs outside the caller's logical bytes.
        unsigned span=(n+24+7)&~7u;if(span<32)span=32;
        for(unsigned i=n;i<span-24;++i)assert(data[i]==0xa5);
        auto barrier=heap.newHandle(31,false);assert(barrier);
        std::memset(*barrier,0x73,31);
        auto p=heap.newPtr(n,true);assert(p);
        for(unsigned i=0;i<n;++i)assert(p[i]==0);
        for(unsigned i=0;i<31;++i)assert((*barrier)[i]==0x73);
        assert(heap.check());
    }
    std::puts("HEAP CLEAR PASS: dirty reuse, zero/short/long extents, padding and neighbouring allocations");
}
