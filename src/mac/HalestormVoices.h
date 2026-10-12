#pragma once
#ifndef POP2_PLATFORM_AMIGA
#include <stdint.h>
#endif

// Adapted from Alone in the Dark's SoundDriver logical/physical separation.
// MIDI Synth 3.45 allocation is re-derived at +$34B6..$3570: occupied
// voices are not stolen; a matching released voice is preferred to a free one.
// This describes original logical voices, independently of Paula's four slots.
class HalestormVoices {
public:
    struct Voice {
        int16_t duration=-1;
        uint16_t instrument=0,note=0,channel=0,owner=0,volume=0,release=0;
        int16_t paula=-1;
        uint32_t serial=0;
    };
    Voice music[5]={},effect={};
    uint16_t musicLimit=0,normalizedLimit=0,effectLimit=0,flags=0;
    uint16_t gain=256,quality=0;
    uint32_t clock=0,sequence=0;
    bool initialized=false;
    const char* initialize(uint16_t musicCount,uint16_t normalized,uint16_t effects) {
        if(musicCount!=5 || normalized!=5 || effects!=1)return "AUDIO / VOICE CONFIGURATION";
        for(auto& voice:music)voice=Voice();effect=Voice();
        musicLimit=musicCount;normalizedLimit=normalized;effectLimit=effects;
        initialized=true;return nullptr;
    }
    const char* setQuality(uint32_t value) {
        if(!initialized)return "AUDIO / NOT INITIALIZED";
        if(value!=11)return "AUDIO / QUALITY CONFIGURATION";
        quality=11;return nullptr;
    }
    void silenceMusic() { for(auto& voice:music)voice.duration=-1; }
    int16_t select(uint16_t instrument,uint16_t note,uint16_t channel,uint16_t owner,bool retrigger) const {
        unsigned active=0;
        for(unsigned i=0;i<musicLimit;++i) {
            const auto& v=music[i];if(v.duration>=0)++active;
            if(v.note==note && v.channel==channel && v.owner==owner && v.instrument==instrument
               && (retrigger || !v.duration))return i;
        }
        if(!(flags&0x4000) || active<normalizedLimit) {
            for(unsigned i=0;i<musicLimit;++i)if(music[i].duration<0)return i;
        } else {
            for(unsigned i=0;i<musicLimit;++i)if(!music[i].duration)return i;
        }
        for(unsigned i=0;i<musicLimit;++i)if(music[i].duration<=0)return i;
        return -1;
    }
    void noteOff(uint16_t note,uint16_t channel,uint16_t owner=0) {
        for(auto& v:music)if(v.duration>=0 && v.note==note && v.channel==channel && v.owner==owner) {
            v.duration=0;v.release=flags&63;
        }
    }
    int16_t physicalOwner(unsigned channel) const {
        for(unsigned i=0;i<musicLimit;++i)if(music[i].paula==int16_t(channel))return i;
        if(effect.paula==int16_t(channel))return 5;
        return -1;
    }
    // D7 donor policy: a free Paula channel first, otherwise oldest music.
    // An effect is never displaced by a music note. Logical admission above
    // remains the original's decision even when Paula cannot render every voice.
    int16_t allocatePaula() const {
        for(unsigned c=0;c<4;++c)if(physicalOwner(c)<0)return c;
        const Voice* oldest=nullptr;
        for(const auto& v:music)if(v.paula>=0 && (!oldest || v.serial<oldest->serial))oldest=&v;
        return oldest ? oldest->paula : -1;
    }
    uint16_t volume(uint16_t midiVolume) const {
        if(!midiVolume || midiVolume>=125)midiVolume=127;
        return uint16_t(((uint32_t(midiVolume)*64+63)/127)*gain>>8);
    }
};
