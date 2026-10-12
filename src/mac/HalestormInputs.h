#pragma once
#ifndef POP2_PLATFORM_AMIGA
#include <stdint.h>
#endif

// Adapted from Alone in the Dark SongInputs. Prince uses format-1 snd resources
// and INST byte-5 velocity/loop flags; the donor's format-2-only contract differs.
namespace HalestormInputs {
inline uint16_t word(const uint8_t* p){return uint16_t(p[0])<<8|p[1];}
inline uint32_t longword(const uint8_t* p){return uint32_t(word(p))<<16|word(p+2);}
struct Instrument {
    uint16_t sample=0,basePitch=0;
    bool velocity=false,disableLoop=false;
    const char* parse(const uint8_t* p,uint32_t size) {
        if(!p || size!=22)return "AUDIO / INSTRUMENT SIZE";
        if(p[4]!=255 || (p[5]&~0x60) || word(p+6) || word(p+8) || word(p+10)
           || word(p+12) || word(p+14) || word(p+16)!=0x8000 || longword(p+18))return "AUDIO / INSTRUMENT FLAGS";
        if(!word(p) || word(p+2)>127)return "AUDIO / INSTRUMENT RANGE";
        sample=word(p);basePitch=word(p+2);velocity=(p[5]&0x40)!=0;disableLoop=(p[5]&0x20)!=0;
        return nullptr;
    }
};
struct Sample {
    const uint8_t* pcm=nullptr;
    uint32_t size=0,rate=0,loopStart=0,loopEnd=0;
    uint8_t baseNote=0;
    const char* parse(const uint8_t* p,uint32_t bytes) {
        if(!p || bytes<42)return "AUDIO / SAMPLE HEADER";
        if(word(p)!=1 || word(p+2)!=1 || word(p+4)!=5 || longword(p+6)!=0xa0
           || word(p+10)!=1 || word(p+12)!=0x8051 || word(p+14) || longword(p+16)!=20)
            return "AUDIO / SAMPLE COMMAND";
        const uint8_t* h=p+20;
        if(longword(h) || h[20] || h[21]>127)return "AUDIO / SAMPLE ENCODING";
        uint32_t n=longword(h+4),start=longword(h+12),end=longword(h+16);
        if(!n || n!=bytes-42 || !longword(h+8) || start>n || end>n || (end && start>=end))return "AUDIO / SAMPLE EXTENT";
        pcm=p+42;size=n;rate=longword(h+8);baseNote=h[21];loopStart=start;loopEnd=end;
        return nullptr;
    }
};
// The quantized pitch table is read from the hash-verified original driver.
// Keep its bytes in ignored runtime data, never in maintained source code.
inline const char* pitch(const Instrument& instrument,const Sample& sample,uint16_t note,
                         const uint8_t* table,uint32_t tableBytes,uint32_t& step) {
    if(!table || tableBytes!=512 || note>127)return "AUDIO / PITCH INPUT";
    int32_t adjusted=note;if(instrument.basePitch)adjusted+=60-instrument.basePitch;
    int32_t index=adjusted+60-sample.baseNote;
    if(index<0 || index>=128)return "AUDIO / PITCH RANGE";
    step=longword(table+4*index);if((step&65535)<4)step&=0xffff0000UL;
    if(!step)return "AUDIO / ZERO PITCH";
    return nullptr;
}
}
