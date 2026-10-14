#include <cstdint>
#include <cassert>
#include <vector>
#include "../src/mac/MacHeap.h"
#include "../src/mac/HandleCopy.h"
int main() {
    for(unsigned state: {0U,0x20U,0x40U,0x60U,0x80U,0xa0U,0xc0U,0xe0U}) {
        std::vector<uint8_t> arena(16384);MacHeap heap;assert(heap.init(arena.data(),arena.size()));
        auto source=heap.newHandle(257);assert(source);
        for(unsigned i=0;i<257;++i)(*source)[i]=uint8_t(i*37+11);
        assert(heap.setState(source,state)==0);MacHeap::Handle copy=nullptr;
        assert(HandleCopy::copy(heap,source,copy)==0 && copy && copy!=source);
        assert(heap.handleSize(copy)==257 && heap.state(source)==state && heap.state(copy)==0);
        for(unsigned i=0;i<257;++i)assert((*copy)[i]==uint8_t(i*37+11));
        (*copy)[0]^=255;assert((*source)[0]==11);assert(heap.check());
    }
    std::vector<uint8_t> arena(4096);MacHeap heap;assert(heap.init(arena.data(),arena.size()));
    auto source=heap.newHandle(2000);assert(source);heap.setState(source,0x40);(*source)[0]=42;
    MacHeap::Handle copy=nullptr;assert(HandleCopy::copy(heap,source,copy)==MacHeap::memFullErr);
    assert(!copy && *source && (*source)[0]==42 && heap.state(source)==0x40 && heap.check());
}
