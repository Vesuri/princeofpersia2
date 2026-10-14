#include <cassert>
#include <cstdint>
#include "mac/MacHeap.h"
int main(){
    alignas(8) static uint8_t arena[65536];MacHeap heap;
    assert(heap.init(arena,sizeof arena));
    auto locked=heap.newHandle(10000),purgeable=heap.newHandle(10000),retained=heap.newHandle(10000);
    assert(locked && purgeable && retained);
    for(unsigned i=0;i<10000;++i){(*locked)[i]=uint8_t(i);(*retained)[i]=uint8_t(i+17);}
    assert(heap.setState(locked,0xc0)==0 && heap.setState(purgeable,0x40)==0);
    auto address=*locked;assert(heap.largestBlock()<40000);
    assert(heap.purge(40000)==0 && heap.error()==0 && heap.largestBlock()>=40000);
    assert(!*purgeable && *locked==address && heap.state(locked)==0xc0 && heap.state(retained)==0);
    for(unsigned i=0;i<10000;++i){assert((*locked)[i]==uint8_t(i));assert((*retained)[i]==uint8_t(i+17));}
    assert(heap.purge(sizeof arena)==MacHeap::memFullErr && heap.error()==MacHeap::memFullErr);
    assert(*locked==address && *retained && heap.check());
}
