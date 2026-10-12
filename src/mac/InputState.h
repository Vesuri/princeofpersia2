#ifndef POP2_INPUT_STATE_H
#define POP2_INPUT_STATE_H
#ifndef POP2_PLATFORM_AMIGA
#include <stdint.h>
#endif
// Single-owner Event Manager state. The platform publishes keyboard edges at
// VBI; readers exclude that interrupt while copying or consuming state.
// Timestamps are supplied by the clock owner, not synthesized by polling calls.
class InputState {
public:
    struct Event { uint16_t what;uint32_t message,when;int16_t v,h;uint16_t modifiers; };
    static constexpr unsigned capacity=64;
    void reset(uint16_t mask=0xffef) { eventMask_=mask; for(auto& b:keys_)b=0;count_=0;overflow_=false; }
    void setEventMask(uint16_t mask){eventMask_=mask;}
    bool down(uint8_t key)const { return key<128 && (keys_[key>>3]&(1u<<(key&7))); }
    void keyMap(uint8_t* out)const {for(unsigned i=0;i<16;++i)out[i]=keys_[i];}
    uint16_t modifiers()const {
        return 0x80 | (down(55)?0x100:0) | (down(56)||down(60)?0x200:0)
          | (down(57)?0x400:0) | (down(58)||down(61)?0x800:0)
          | (down(59)||down(62)?0x1000:0);
    }
    bool edge(uint8_t key,bool pressed,uint8_t character,uint32_t ticks) {
        if(key>=128)return false;
        if(down(key)==pressed)return !overflow_;
        const uint8_t bit=1u<<(key&7);
        if(pressed)keys_[key>>3]|=bit;else keys_[key>>3]&=uint8_t(~bit);
        // Modifier transitions change levels, without keyboard EventRecords.
        if(key>=55 && key<=62)return !overflow_;
        if(!(eventMask_&(1u<<(pressed?3:4))))return !overflow_;
        if(count_==capacity){overflow_=true;return false;}
        events_[count_++]={uint16_t(pressed?3:4),uint32_t(0x00020000)|(uint32_t(key)<<8)|character,ticks,0,0,modifiers()};
        return !overflow_;
    }
    bool event(uint16_t mask,bool remove,uint32_t ticks,Event& out) {
        for(unsigned i=0;i<count_;++i)if(mask&(1u<<events_[i].what)) {
            out=events_[i];if(remove){for(unsigned j=i+1;j<count_;++j)events_[j-1]=events_[j];--count_;}return true;
        }
        out={0,0,ticks,0,0,modifiers()};return false;
    }
    void flush(uint16_t which,uint16_t stop) {
        unsigned retained=0;bool stopped=false;
        for(unsigned i=0;i<count_;++i) {
            const uint16_t bit=1u<<events_[i].what;
            if(stop&bit)stopped=true;
            if(stopped || !(which&bit))events_[retained++]=events_[i];
        }
        count_=retained;
    }
    bool overflow()const{return overflow_;}
    unsigned queued()const{return count_;}
private:
    uint8_t keys_[16]={};Event events_[capacity]={};unsigned count_=0;bool overflow_=false;uint16_t eventMask_=0xffef;
};
#endif
