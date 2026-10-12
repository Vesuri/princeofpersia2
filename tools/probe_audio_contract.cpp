// Original-CPU state pairs are supplied from ignored reference output.
#include <cstdio>
#include <cstdlib>
#include <vector>
#include "mac/HalestormVoices.h"
#include "mac/HalestormInputs.h"
#include "mac/HalestormEngine.h"
using namespace HalestormInputs;
static std::vector<uint8_t> read(const char* root,const char* side,unsigned index) {
    char path[1024];snprintf(path,sizeof path,"%s/audio-fixture-%s-%u.bin",root,side,index);
    FILE* f=fopen(path,"rb");if(!f)exit(2);std::vector<uint8_t> out(0x32f0);
    if(fread(out.data(),1,out.size(),f)!=out.size() || fgetc(f)!=EOF)exit(3);fclose(f);return out;
}
static bool compare(const HalestormVoices& model,const std::vector<uint8_t>& data) {
    for(unsigned i=0;i<5;++i) {
        const uint8_t* p=data.data()+0x24f2+4*i;const auto& v=model.music[i];
        if(v.duration!=int16_t(word(p+0x200)) || v.note!=word(p+0x480)
           || v.instrument!=word(p+0x4c0) || v.channel!=word(p+0x440) || v.owner!=word(p+0x400))return false;
    }
    return true;
}
int main(int argc,char** argv) {
    if(argc!=2)return 2;
    HalestormVoices model;auto first=read(argv[1],"before",1);const auto* p=first.data();
    if(model.initialize(word(p+0x13d6),word(p+0x13d8),word(p+0x13da)))return 3;
    model.flags=word(p+0x13d0);
    for(unsigned i=0;i<5;++i) {
        const auto* v=p+0x24f2+4*i;auto& dest=model.music[i];
        dest.duration=int16_t(word(v+0x200));dest.note=word(v+0x480);dest.instrument=word(v+0x4c0);
        dest.channel=word(v+0x440);dest.owner=word(v+0x400);
    }
    const unsigned selectors[]={6,9,9,9,9,9,9,12,9,9,12,9,38};
    const unsigned notes[]={0,60,61,62,63,64,65,62,66,63,63,63,0};
    unsigned dropped=0,reused=0;
    for(unsigned i=0;i<13;++i) {
        auto before=read(argv[1],"before",i+1);auto after=read(argv[1],"after",i+1);
        if(!compare(model,before)){fprintf(stderr,"AUDIO fixture %u before mismatch\n",i+1);return 4;}
        int slot=-1;
        if(selectors[i]==6)model.silenceMusic();
        else if(selectors[i]==12)model.noteOff(notes[i],0);
        else if(selectors[i]==9) {
            slot=model.select(0,notes[i],0,0,false);
            if(slot<0)++dropped;
            else {auto& v=model.music[slot];if(!v.duration)++reused;v.duration=20000;v.note=notes[i];v.instrument=v.channel=v.owner=0;}
        }
        if(!compare(model,after)){fprintf(stderr,"AUDIO fixture %u after mismatch slot=%d\n",i+1,slot);return 5;}
    }
    if(dropped!=2 || reused!=2)return 6;
    HalestormEngine engine;
    if(engine.configure(5,5,1))return 7;
    engine.voices.flags=model.flags;
    auto start=read(argv[1],"after",15);const auto* original=start.data()+0x24f2;
    auto& v=engine.voices.music[0];v.duration=20000;v.note=60;v.release=model.flags&63;
    engine.cursors[0]={0,0,longword(original+0x40),true};
    uint32_t base=longword(original);
    auto& sample=engine.sounds[0].sample;sample.size=longword(original+0x280)-base+1;
    sample.loopStart=longword(original+0x2c0)?longword(original+0x2c0)-base:0;
    sample.loopEnd=longword(original+0x300)?longword(original+0x300)-base:0;
    for(unsigned index=16;index<=27;++index){
        if(index==19)engine.voices.noteOff(60,0);else engine.advance();
        auto after=read(argv[1],"after",index);const auto* state=after.data()+0x24f2;
        if(v.duration!=int16_t(word(state+0x200)) || v.release!=word(state+0x500))return 8;
        if(v.duration>=0 && engine.cursors[0].position!=longword(state)-base){fprintf(stderr,"release cursor %u: %u/%u\n",index,engine.cursors[0].position,longword(state)-base);return 9;}
    }
    // Physical pressure follows the donor policy independently of the five
    // original logical admissions: oldest music goes, never the effect.
    HalestormVoices physical;physical.initialize(5,5,1);
    for(unsigned i=0;i<4;++i){physical.music[i].paula=physical.allocatePaula();physical.music[i].serial=i+1;}
    if(physical.allocatePaula()!=0)return 10;
    physical.music[0].paula=-1;physical.effect.paula=0;
    if(physical.allocatePaula()!=1)return 11;
    physical.music[1].paula=-1;if(physical.allocatePaula()!=1)return 12;
    printf("AUDIO ALLOCATION PAIR PASS: 27 original calls, 2 drops, 2 reuses, release lifetime/cursors and DMA pressure\n");
}
