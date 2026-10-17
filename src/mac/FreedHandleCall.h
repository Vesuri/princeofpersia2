#ifndef POP2_FREED_HANDLE_CALL_H
#define POP2_FREED_HANDLE_CALL_H
// Game-specific supported call sites, never a general Memory Manager rule.
// System 7.5.5 classifies freed masters from incidental heap bytes and can even
// mutate their free links. Only the original-paired aliases below are supported;
// the caller must independently prove this address is a currently free slot.
namespace FreedHandleCall {
inline bool supported(uint16_t trap, uint32_t segment, uint32_t offset)
{
    return (trap==0xa049 && segment==3 && (offset==0x5f9a || offset==0x63aa)) ||
           (trap==0xa049 && segment==15 &&
            (offset==0x29b0 || offset==0x29c8 || offset==0x29e0)) ||
           (trap==0xa02a && segment==15 &&
            (offset==0x29aa || offset==0x29c2 || offset==0x29da));
}
}
#endif
