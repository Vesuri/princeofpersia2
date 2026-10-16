#ifndef AMIGAXDEV_LINE_A_VECTOR_H
#define AMIGAXDEV_LINE_A_VECTOR_H
// WHDLF_EmulLineA forwards to the guest $28. The real VBR belongs to
// WHDLoad and must never be patched by the hosted Macintosh runtime.
// Extracted from Alone in the Dark's installLineAVector policy.
namespace MacRuntime {
inline unsigned long lineAVectorAddress(bool whdload, unsigned long nativeVBR) {
    return whdload ? 0x28UL : nativeVBR + 0x28UL;
}
}
#endif
