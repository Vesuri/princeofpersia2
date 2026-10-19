#pragma once
#include "HalestormInputs.h"
#include "PaulaSample.h"

// Adapted from Alone in the Dark SongVoice's integer period calculation and
// phase-preserving DMA preparation. The pitch comes from a verified input.
namespace HalestormDma {
#include "UnsignedDivide.inc"
struct Plan {PaulaSample::Layout layout={};uint16_t period=0,stride=1;};
inline const char* describe(const HalestormInputs::Sample& sample,uint32_t step,bool looping,Plan& out) {
    if(!sample.pcm || !sample.size || !step || sample.rate!=0x2b7745d1UL)return "AUDIO / MUSIC RATE";
    out={};unsigned long long denominator=static_cast<unsigned long long>(sample.rate)*step;
    unsigned long long numerator=static_cast<unsigned long long>(3546895)<<32;uint32_t period=0;
    if(!divide(numerator+(denominator>>1),denominator,period))return "AUDIO / PERIOD DIVISION";
    while(period<124 && out.stride<16){out.stride*=2;numerator*=2;if(!divide(numerator+(denominator>>1),denominator,period))return "AUDIO / PERIOD DIVISION";}
    if(period<124 || period>65535)return "AUDIO / MUSIC PERIOD";out.period=period;
    auto& l=out.layout;l.pcm=sample.pcm;l.size=sample.size;
    if(looping && sample.loopStart && sample.loopEnd && uint16_t(sample.loopEnd-sample.loopStart)>=100){l.loopStart=sample.loopStart;l.loopEnd=sample.loopEnd;}
    uint32_t end=l.loopEnd?l.loopEnd:l.size;
    l.attackBytes=(((end+out.stride-1)/out.stride)+1)&~1UL;l.reloadOffset=l.attackBytes;
    if(l.loopEnd){
        uint32_t loop=l.loopEnd-l.loopStart,divisor=out.stride;
        while(divisor>1 && loop%divisor)divisor/=2;
        uint32_t cycle=loop/divisor;l.reloadBytes=cycle&1 ? cycle*2:cycle;
    }else l.reloadBytes=2;
    l.allocated=l.attackBytes+l.reloadBytes;
    if(l.attackBytes>131070 || l.reloadBytes>131070 || l.allocated>131072)return "AUDIO / MUSIC DMA EXTENT";
    return nullptr;
}
using Dma=Plan;
#include "PcmDma.inc"

}
