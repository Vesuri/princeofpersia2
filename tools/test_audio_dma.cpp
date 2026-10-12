// Synthetic phase/alignment/bounds controls; no original samples or pitch table.
#include <cstdint>
#include <cstdio>
#include <vector>
#include "mac/HalestormDma.h"
#include "mac/SoundEffect.h"
int main(){
    uint32_t random=17;
    for(unsigned i=0;i<2000;++i){
        random=random*1664525u+1013904223u;
        unsigned long long n=(static_cast<unsigned long long>(random)<<28)|(random>>4),d=random|1;
        uint32_t answer=0;bool ok=HalestormDma::divide(n,d,answer);
        if(ok!=(n/d<=0xffffffffULL) || (ok && answer!=n/d))return 1;
    }
    std::vector<uint8_t> pcm(513);for(unsigned i=0;i<pcm.size();++i)pcm[i]=i*37;
    unsigned cases=0;
    for(uint32_t step: {0x1000u,0x10000u,0x30000u,0x70000u,0x170000u})
    for(uint32_t end: {0u,106u,107u,210u}){
        HalestormInputs::Sample s;s.pcm=pcm.data();s.size=pcm.size();s.rate=0x2b7745d1;s.loopStart=end?3:0;s.loopEnd=end;
        HalestormDma::Plan p;if(HalestormDma::describe(s,step,true,p))return 2;
        std::vector<uint8_t> out(p.layout.allocated+16,0x5a);HalestormDma::convert(p,out.data());
        for(unsigned i=p.layout.allocated;i<out.size();++i)if(out[i]!=0x5a)return 3;
        unsigned long long denominator=static_cast<unsigned long long>(s.rate)*step;
        if(p.period!=((3546895ULL<<32)*p.stride+denominator/2)/denominator || p.period<124)return 4;
        for(uint32_t i=0;i<p.layout.attackBytes+4*p.layout.reloadBytes;++i){
            uint32_t at=i<p.layout.attackBytes?i:p.layout.reloadOffset+(i-p.layout.attackBytes)%p.layout.reloadBytes;
            uint32_t source=i*p.stride;uint8_t expected=0;
            if(end){if(source>=end)source=3+(source-end)%(end-3);expected=pcm[source]^128;}
            else if(i<p.layout.attackBytes && source<pcm.size())expected=pcm[source]^128;
            if(out[at]!=expected)return 5;
        }
        ++cases;
    }
    PaulaSample::Layout layout;uint16_t period=0;uint32_t ticks=0;
    if(!SoundEffect::describe(pcm.data(),pcm.size(),0x2b110000,1,100,layout,period,ticks,3546895))return 6;
    if(!SoundEffect::describe(pcm.data(),131071,0x2b110000,0,0,layout,period,ticks,3546895))return 7;
    if(SoundEffect::describe(pcm.data(),pcm.size(),0x2b110000,0,0,layout,period,ticks,3546895))return 8;
    if(period!=322 || layout.reloadBytes!=2 || layout.attackBytes!=514)return 9;
    printf("AUDIO DMA SYNTHETIC PASS: %u phase/stride cases, integer division and extent/loop controls\n",cases);
}
