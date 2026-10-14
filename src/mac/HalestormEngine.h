#pragma once
#include "HalestormInputs.h"
#include "HalestormVoices.h"

// Original logical bookkeeping without a software mixer. PCM is rendered only
// by Paula. The application retains Mohawk archive loading and MIDI sequencing.
class HalestormEngine {
public:
    using Lookup=bool(*)(uint32_t,int16_t,const uint8_t*&,uint32_t&);
    struct Sound {HalestormInputs::Instrument instrument;HalestormInputs::Sample sample;bool loaded=false;};
    struct Cursor {uint32_t position=0,fraction=0,step=0;bool first=false;};
    Sound sounds[34]={};Cursor cursors[6]={};
    HalestormVoices voices;
    const uint8_t* pitchTable=nullptr;
    uint32_t clock=0;
    const uint8_t* effectPCM=nullptr;uint32_t effectBytes=0,effectRate=0;uint16_t effectID=0;
    int16_t selected=-1;
    uint32_t completed=0;
    const char* prepare(const uint8_t* table,Lookup resource) {
        if(!table || !resource)return "AUDIO / INPUTS";
        pitchTable=table;
        for(unsigned i=0;i<34;++i) {
            const uint8_t* p=nullptr;uint32_t n=0;
            if(!resource(0x494e5354,i,p,n))return "AUDIO / INSTRUMENT RESOURCE";
            const char* why=sounds[i].instrument.parse(p,n);if(why)return why;
            if(!resource(0x736e6420,sounds[i].instrument.sample,p,n))return "AUDIO / SAMPLE RESOURCE";
            if((why=sounds[i].sample.parse(p,n)))return why;
        }
        return nullptr;
    }
    const char* configure(uint16_t music,uint16_t normalized,uint16_t effects) {
        const char* why=voices.initialize(music,normalized,effects);if(why)return why;
        for(auto& c:cursors)c=Cursor();return nullptr;
    }
    const char* noteOn(uint16_t instrument,uint16_t note,uint16_t duration,uint16_t volume,uint16_t channel) {
        selected=-1;
        if(!voices.initialized)return "AUDIO / NOT INITIALIZED";
        if(instrument>=34)return "AUDIO / INSTRUMENT ID";
        if(note>127 || volume>127 || !duration || duration>32767 || channel>15)return "AUDIO / NOTE PACKET";
        // MIDI Synth 3.45 +33AE/+33B2 returns without allocating a voice
        // when the instrument slot is empty. Loaded/reloaded controls and
        // the level-four instrument-32 request establish this path.
        if(!sounds[instrument].loaded)return nullptr;
        const auto& sound=sounds[instrument];uint32_t step=0;
        const char* why=HalestormInputs::pitch(sound.instrument,sound.sample,note,pitchTable,512,step);if(why)return why;
        selected=voices.select(instrument,note,channel,0,false);
        if(selected<0)return nullptr;
        auto& v=voices.music[selected];auto& c=cursors[selected];
        v.instrument=instrument;v.note=note;v.channel=channel;v.owner=0;v.duration=duration;
        v.volume=sound.instrument.velocity ? volume : 0;v.release=voices.flags&63;v.serial=++voices.sequence;
        c={0,0,step,true};return nullptr;
    }
    const char* effectOn(const uint8_t* pcm,uint32_t size,uint32_t rate,uint16_t id) {
        if(!voices.initialized || !pcm || !size || size>131070)return "AUDIO / EFFECT INPUT";
        uint32_t quotient=(rate>>5)/0x2b77;
        if(!quotient || quotient>65535)return "AUDIO / EFFECT RATE";
        uint32_t step=quotient<<5;
        if((step&65535)==32)step&=0xffff0000UL;
        if((step&65535)==0xffe0)step+=32;
        if(!step)return "AUDIO / EFFECT ZERO STEP";
        effectPCM=pcm;effectBytes=size;effectRate=rate;effectID=id;
        voices.effect.duration=0x7ffe;voices.effect.serial=++voices.sequence;
        cursors[5]={0,0,step,false};return nullptr;
    }
    // Advance the source cursors by the measured 185 output samples per
    // low-quality synthesis block. This never reads, sums or mixes PCM bytes.
    void advance() {
        ++clock;
        for(unsigned i=0;i<5;++i) {
            auto& v=voices.music[i];auto& c=cursors[i];if(v.duration<0)continue;
            if(c.first){c.first=false;continue;}
            if(v.duration) --v.duration;
            else if(int16_t(v.release)>=0) --v.release;
            else {v.duration=-1;completed|=1u<<i;continue;}
            const auto& sound=sounds[v.instrument];const auto& sample=sound.sample;
            uint32_t increment=(c.step&65535)*185+c.fraction;
            c.position+=(c.step>>16)*185+(increment>>16);c.fraction=increment&65535;
            bool loop=!sound.instrument.disableLoop && v.duration && sample.loopStart && sample.loopEnd
                && uint16_t(sample.loopEnd-sample.loopStart)>=100;
            if(loop && c.position>=sample.loopEnd)c.position=sample.loopStart+(c.position-sample.loopEnd)%(sample.loopEnd-sample.loopStart);
            if(!loop && c.position>=sample.size-1){v.duration=-1;completed|=1u<<i;}
        }
        if(voices.effect.duration>=0) {
            auto& c=cursors[5];--voices.effect.duration;
            uint32_t increment=(c.step&65535)*185+c.fraction;
            c.position+=(c.step>>16)*185+(increment>>16);c.fraction=increment&65535;
            if(c.position>=effectBytes){voices.effect.duration=-1;completed|=32;}
        }
    }
};
