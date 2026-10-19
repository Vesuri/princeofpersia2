// Included inside a caller namespace after defining Dma (layout and stride).
// Canonical Alone in the Dark PCM stream conversion; intentionally no include guard.
inline uint8_t pcmAt(const Dma& dma,uint32_t position) {
    const auto& layout=dma.layout;
    if(layout.loopEnd && position>=layout.loopEnd)
        position=layout.loopStart+(position-layout.loopEnd)%(layout.loopEnd-layout.loopStart);
    return position<layout.size ? layout.pcm[position]^0x80 : 0;
}
inline void convertSpan(const Dma& dma,uint8_t* destination,uint32_t bytes,uint32_t& position) {
    const auto& layout=dma.layout;
    while(bytes) {
        if(layout.loopEnd && position>=layout.loopEnd)
            position=layout.loopStart+(position-layout.loopEnd)%(layout.loopEnd-layout.loopStart);
        if(position>=layout.size) {
            while(bytes--) *destination++=0;
            return;
        }
        // Copy an uninterrupted source span. Bounds and loop arithmetic belong
        // at its boundary, not in the per-byte unsigned-to-signed conversion.
        uint32_t end=layout.loopEnd ? layout.loopEnd : layout.size;
        uint32_t count=1+(end-1-position)/dma.stride;
        if(count>bytes)count=bytes;
        bytes-=count;
        // Ordinary PCM needs only a sign-bit flip. The 68020+ permits unaligned
        // longword transfers; use explicit instructions to avoid byte copies.
        if(dma.stride==1) {
#if defined(__m68k__)
            uint32_t chunks=count>>5;
            if(chunks) {
                const uint8_t* source=layout.pcm+position;
                uint32_t word;
                uint32_t copied=chunks<<5;
                __asm__ volatile(
                    "1: .rept 8\n\t"
                    "move.l (%0)+,%3\n\teor.l %4,%3\n\tmove.l %3,(%1)+\n\t"
                    ".endr\n\tsubq.l #1,%2\n\tbne.s 1b"
                    : "+a"(source),"+a"(destination),"+d"(chunks),"=&d"(word)
                    : "d"(0x80808080UL) : "cc","memory");
                position+=copied;count-=copied;
            }
#endif
          while(count>=4) {
            uint32_t word;
#if defined(__m68k__)
            __asm__ volatile("move.l (%1),%0\n\t"
                             "eor.l #0x80808080,%0\n\t"
                             "move.l %0,(%2)"
                             : "=&d"(word)
                             : "a"(layout.pcm+position),"a"(destination)
                             : "cc","memory");
#else
            __builtin_memcpy(&word,layout.pcm+position,4);
            word^=0x80808080UL;
            __builtin_memcpy(destination,&word,4);
#endif
            position+=4;destination+=4;count-=4;
          }
        }
        while(count--) {
            *destination++=layout.pcm[position]^0x80;
            position+=dma.stride;
        }
    }
}
inline void convert(const Dma& dma,uint8_t* destination) {
    const auto& layout=dma.layout;
    uint32_t position=0;
    convertSpan(dma,destination,layout.attackBytes,position);
    if(!layout.loopEnd) {
        destination[layout.reloadOffset]=destination[layout.reloadOffset+1]=0;return;
    }
    // Decimate the source stream, including its wrap. Odd loops retain their
    // complete phase cycle; pad/reload alignment never repeats or loses a byte.
    convertSpan(dma,destination+layout.reloadOffset,layout.reloadBytes,position);
}
