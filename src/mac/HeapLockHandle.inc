// Include in a MacHeap implementation. The validated master slot is looked up
// once; no pointer, block or free-list metadata changes. Preserve the existing
// zone header; report MemError through the same query policy as state().
int16_t MacHeap::lockHandle(Handle h)
{
    uint8_t* f=flags(h);
    if(!f || !(*f&1))return queryResult(nilHandleErr);
    *f=(*f&0xe0)|0x81;
    return queryResult(0);
}
