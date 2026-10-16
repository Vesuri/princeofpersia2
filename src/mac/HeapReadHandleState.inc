// Include in a MacHeap implementation. Read state after one validated master
// slot lookup, preserving the read-only query's error and zone-header policy.
uint8_t MacHeap::state(Handle h)
{
    uint8_t* f=flags(h);
    if(!f || !(*f&1)) { queryResult(nilHandleErr);return 0; }
    uint8_t value=*f&0xe0;
    queryResult(0);
    return value;
}
