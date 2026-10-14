#ifndef AMIGAXDEV_HANDLE_COPY_H
#define AMIGAXDEV_HANDLE_COPY_H
// Copy owned handle data without allowing allocation to purge the source.
// The caller rejects unmeasured invalid/purged handles before invoking this.
namespace HandleCopy {
template<class Heap>
int16_t copy(Heap& heap,typename Heap::Handle source,typename Heap::Handle& result) {
    result=nullptr;
    if(!heap.isHandle(source) || !*source)return Heap::nilHandleErr;
    const uint32_t size=heap.handleSize(source);
    const uint8_t state=heap.state(source);
    int16_t error=heap.setState(source,state|0x80);
    if(error)return error;
    auto target=heap.newHandle(size,false);
    error=heap.error();
    if(target)for(uint32_t i=0;i<size;++i)(*target)[i]=(*source)[i];
    const int16_t restored=heap.setState(source,state);
    if(restored){if(target)heap.disposeHandle(target);return restored;}
    if(!target)return error;
    result=target;return 0;
}
}
#endif
