#ifndef AITD_PAULA_SAMPLE_H
#define AITD_PAULA_SAMPLE_H

// Fixed-width types come from the including translation unit (Amiga compat types).

// Build word-aligned DMA segments without changing the original PCM sequence.
namespace PaulaSample {
struct Layout {
    const uint8_t* pcm;
    uint32_t size, attackBytes, reloadOffset, reloadBytes, allocated;
    uint32_t loopStart, loopEnd;
    uint16_t rate;
};
inline uint16_t word(const uint8_t* p) { return (uint16_t)((p[0] << 8) | p[1]); }
inline bool describe(const uint8_t* data, uint32_t bytes, bool repeatRaw, Layout& out)
{
    out = {};
    out.pcm = data;
    out.size = bytes;
    if (bytes > 8 && word(data + 6) == bytes - 8) {
        out.pcm += 8;
        out.size -= 8;
        out.rate = word(data + 4);
        uint32_t start = word(data), end = word(data + 2);
        if (end > start && end <= out.size) {
            out.loopStart = start;
            out.loopEnd = end;
        }
    } else if (repeatRaw) {
        out.loopEnd = bytes;
    }
    if (!out.size || out.size > 131070UL) return false;
    uint32_t attack = out.loopEnd ? out.loopEnd : out.size;
    out.attackBytes = (attack + 1) & ~1UL;
    out.reloadOffset = out.attackBytes;
    uint32_t loopBytes = out.loopEnd - out.loopStart;
    // Two copies of an odd-sized loop make a complete number of DMA words.
    out.reloadBytes = loopBytes ? ((loopBytes & 1) ? (loopBytes << 1) : loopBytes) : 2;
    if (out.reloadBytes > 131070UL) return false;
    out.allocated = out.attackBytes + out.reloadBytes;
    if (loopBytes && !(out.loopStart & 1) && !(out.loopEnd & 1)) {
        out.reloadOffset = out.loopStart;
        out.allocated = out.attackBytes;
    }
    return true;
}
inline void convert(const Layout& layout, uint8_t* destination)
{
    uint32_t attack = layout.loopEnd ? layout.loopEnd : layout.size;
    for (uint32_t i = 0; i < attack; ++i) destination[i] = layout.pcm[i] ^ 0x80;
    if (layout.attackBytes != attack)
        destination[attack] = layout.loopEnd ? (layout.pcm[layout.loopStart] ^ 0x80) : 0;
    if (layout.reloadOffset < layout.attackBytes) return;
    uint8_t* reload = destination + layout.reloadOffset;
    if (!layout.loopEnd) {
        reload[0] = reload[1] = 0;
        return;
    }
    // If alignment consumed the first loop byte, rotate the repeated segment
    // by one byte. Both the attack-to-loop and subsequent seams stay exact.
    uint32_t cursor = layout.loopStart + (attack & 1);
    if (cursor == layout.loopEnd) cursor = layout.loopStart;
    for (uint32_t i = 0; i < layout.reloadBytes; ++i) {
        reload[i] = layout.pcm[cursor++] ^ 0x80;
        if (cursor == layout.loopEnd) cursor = layout.loopStart;
    }
}
}
#endif
