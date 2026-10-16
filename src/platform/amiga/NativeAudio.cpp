#ifdef POP2_AUDIO
#include <proto/exec.h>
#include <exec/memory.h>
#include "NativeAudio.h"
#include "PaulaAudio.h"
#include "mac/HalestormDma.h"
#include "mac/SoundEffect.h"
#include "mac/ImmutablePcmCache.h"
using namespace HalestormInputs;
namespace {
HalestormEngine engine;
struct SampleMemory {
    static uint8_t* allocate(uint32_t n){return (uint8_t*)AllocMem(n,MEMF_CHIP);}
    static void free(uint8_t* p,uint32_t n){FreeMem(p,n);}
};
struct SampleConversion {
    static uint32_t bytes(const HalestormDma::Plan& p){return p.layout.allocated;}
    static bool equal(const HalestormDma::Plan& a,const HalestormDma::Plan& b){
        const auto& x=a.layout;const auto& y=b.layout;
        return a.stride==b.stride && x.pcm==y.pcm && x.size==y.size
            && x.loopStart==y.loopStart && x.loopEnd==y.loopEnd
            && x.attackBytes==y.attackBytes && x.reloadOffset==y.reloadOffset
            && x.reloadBytes==y.reloadBytes && x.allocated==y.allocated;
    }
    static void convert(const HalestormDma::Plan& p,uint8_t* data){HalestormDma::convert(p,data);}
};
// These samples point into the owned, immutable application resource image,
// not movable game handles. Mutable effects and cursor-specific release tails
// continue to use the four channel buffers.
using SampleCache=ImmutablePcmCache<HalestormDma::Plan,SampleMemory,SampleConversion>;
SampleCache sampleCache;
SampleCache::Entry* channelSamples[4]={};
void stopChannel(unsigned channel){
    PaulaAudio::stop(channel);
    sampleCache.release(channelSamples[channel]);channelSamples[channel]=nullptr;
}
void replaceChannel(unsigned channel){
    PaulaAudio::halt(channel);
    sampleCache.release(channelSamples[channel]);channelSamples[channel]=nullptr;
}
NativeAudio::Validate valid=nullptr;
const char* failure=nullptr;
volatile uint32_t fieldsClock=0;
uint32_t observedClock=0;uint16_t fraction=0;
bool replaying=false,suspended=false;
uint32_t musicCallback=0,musicContext=0,effectCallback=0,effectContext=0;
struct Callback {uint32_t code,context;} callbacks[32]={};unsigned callbackRead=0,callbackWrite=0;
bool fail(const char* why){failure=why;return false;}
bool queue(uint32_t code,uint32_t context){
    if(!code)return true;
    if(callbackWrite-callbackRead==32)return fail("AUDIO / CALLBACK QUEUE FULL");
    callbacks[(callbackWrite++)&31]={code,context};return true;
}
void detach(unsigned channel){
    for(auto& v:engine.voices.music)if(v.paula==int16_t(channel))v.paula=-1;
    if(engine.voices.effect.paula==int16_t(channel))engine.voices.effect.paula=-1;
}
void stopVoice(HalestormVoices::Voice& v){if(v.paula>=0){stopChannel(v.paula);v.paula=-1;}}
bool completed(){
    uint32_t mask=engine.completed;engine.completed=0;
    for(unsigned i=0;i<5;++i)if(mask&(1u<<i)){stopVoice(engine.voices.music[i]);if(!queue(musicCallback,musicContext))return false;}
    if(mask&32){stopVoice(engine.voices.effect);if(!queue(effectCallback,effectContext))return false;effectCallback=0;}
    return true;
}
bool musicStart(unsigned slot){
    auto& voice=engine.voices.music[slot];const auto& sound=engine.sounds[voice.instrument];
    int channel=voice.paula;
    if(channel<0)channel=engine.voices.allocatePaula();
    if(channel<0)return fail("AUDIO / PAULA ALLOCATION");
    replaceChannel(channel);detach(channel);
    HalestormDma::Plan plan;
    const char* why=HalestormDma::describe(sound.sample,engine.cursors[slot].step,!sound.instrument.disableLoop,plan);
    if(why)return fail(why);
    auto* cached=sampleCache.acquire(plan);channelSamples[channel]=cached;
    if(!cached)HalestormDma::convert(plan,PaulaAudio::buffer(channel));
    voice.paula=channel;
    if(!PaulaAudio::start(channel,plan.layout,plan.period,engine.voices.volume(voice.volume),0,0,cached?cached->data:nullptr))return fail(PaulaAudio::error());
    return true;
}
bool effectStop(bool notify){
    bool active=engine.voices.effect.duration>=0;stopVoice(engine.voices.effect);engine.voices.effect.duration=-1;
    if(active && notify && !queue(effectCallback,effectContext))return false;
    effectCallback=0;return true;
}
}
namespace NativeAudio {
const char* error(){return failure;}
const HalestormEngine& state(){return engine;}
bool prepare(const uint8_t* pitch,HalestormEngine::Lookup lookup,Validate validate){
    valid=validate;failure=nullptr;suspended=false;engine=HalestormEngine();callbackRead=callbackWrite=0;
    const char* why=engine.prepare(pitch,lookup);if(why)return fail(why);
    if(!PaulaAudio::prepare())return fail(PaulaAudio::error());return true;
}
void release(){for(unsigned c=0;c<4;++c)stopChannel(c);sampleCache.clearUnused();PaulaAudio::release();}
void take(){fieldsClock=observedClock=0;fraction=0;PaulaAudio::take();if(PaulaAudio::error())failure=PaulaAudio::error();}
void restore(){PaulaAudio::restore();}
void field(){fraction+=6;fieldsClock+=fraction/5;fraction%=5;}
bool servicePending(){return failure || callbackRead!=callbackWrite || fieldsClock!=observedClock;}
bool sync(){
    if(failure)return false;if(replaying)return true;
    uint32_t now=fieldsClock,elapsed=now-observedClock;observedClock=now;
    if(elapsed>36000)return fail("AUDIO / CLOCK BACKLOG");
    while(elapsed--)if(engine.voices.initialized && !suspended){engine.advance();if(!completed())return false;}
    return true;
}
bool callback(uint32_t& code,uint32_t& context){
    if(callbackRead==callbackWrite)return false;
    auto c=callbacks[(callbackRead++)&31];code=c.code;context=c.context;return true;
}
bool dispatch(uint32_t selector,uint32_t argument,uint32_t& result,bool& retry){
    retry=false;
    if(!sync())return false;result=0;engine.selected=-1;
    if(selector!=21 && !engine.voices.initialized)return fail("AUDIO / NOT INITIALIZED");
    unsigned extent=selector==9 || selector==12 ? 10 : selector==21 ? 6 : selector==27 ? 8 : selector==17 || selector==18 || selector==20 ? 38 : 0;
    if(extent && (!valid || !valid(argument,extent)))return fail("AUDIO / PACKET EXTENT");
    const uint8_t* p=(const uint8_t*)argument;const char* why=nullptr;
    switch(selector){
    case 8:
        // Alone in the Dark's close-driver policy, paired here with MIDI Synth
        // 3.45 +4358: stop music/effects, retire callbacks and release cached
        // sample ownership. Platform restoration retains the Paula registers.
        for(auto& v:engine.voices.music)stopVoice(v);
        engine.voices.silenceMusic();if(!effectStop(false))return false;
        for(unsigned c=0;c<4;++c)stopChannel(c);
        sampleCache.clearUnused();callbackRead=callbackWrite=0;
        musicCallback=musicContext=effectCallback=effectContext=0;
        engine.voices.initialized=false;suspended=false;break;
    case 6:for(auto& v:engine.voices.music)stopVoice(v);engine.voices.silenceMusic();break;
    case 9:
        if(suspended)return fail("AUDIO / NOTE DURING SUSPEND");
        why=engine.noteOn(word(p),word(p+2),word(p+4),word(p+6),word(p+8));
        if(why)return fail(why);if(engine.selected>=0 && !musicStart(engine.selected))return false;break;
    case 10:
        // MIDI Synth 3.45 +090A removes its output timer. The restart
        // route first stops every voice; preserve the logical state and
        // discard wall-clock time while suspended, just like that timer.
        // Active-voice suspension needs its own original/Paula contract.
        for(const auto& v:engine.voices.music)if(v.duration>=0 || v.paula>=0)return fail("AUDIO / ACTIVE MUSIC SUSPEND");
        if(engine.voices.effect.duration>=0 || engine.voices.effect.paula>=0)return fail("AUDIO / ACTIVE EFFECT SUSPEND");
        suspended=true;break;
    case 11:suspended=false;break;
    case 12:
        // The original stops wrapping at loopEnd on note-off and reads the
        // remaining source tail. A physical voice follows the same source
        // position; its logical release deadline remains independent of DMA.
        for(unsigned slot=0;slot<5;++slot){
            const auto& voice=engine.voices.music[slot];const auto& sound=engine.sounds[voice.instrument];
            const auto& sample=sound.sample;uint32_t position=engine.cursors[slot].position;
            if(voice.duration<0 || voice.note!=word(p+2) || voice.channel!=word(p+8) || voice.owner || voice.paula<0
               || sound.instrument.disableLoop || !sample.loopStart || !sample.loopEnd || uint16_t(sample.loopEnd-sample.loopStart)<100)continue;
            if(position>=sample.size)return fail("AUDIO / RELEASE CURSOR");
            auto tail=sample;tail.pcm+=position;tail.size-=position;tail.loopStart=tail.loopEnd=0;
            HalestormDma::Plan plan;why=HalestormDma::describe(tail,engine.cursors[slot].step,false,plan);if(why)return fail(why);
            replaceChannel(voice.paula);HalestormDma::convert(plan,PaulaAudio::buffer(voice.paula));
            if(!PaulaAudio::start(voice.paula,plan.layout,plan.period,engine.voices.volume(voice.volume),slot+1,position))return fail(PaulaAudio::error());
        }
        engine.voices.noteOff(word(p+2),word(p+8));break;

    case 15:result=engine.clock;break;
    case 16:engine.clock=argument;break;
    case 17:{
        if(suspended)return fail("AUDIO / EFFECT DURING SUSPEND");
        // MIDI Synth 3.45 +37AC completes the old effect before reading
        // the replacement descriptor. Its callback may change that descriptor.
        // Return through the existing user-mode callback trampoline, then
        // retry this call with the original stack arguments still in place.
        if(!effectStop(true))return false;
        if(callbackRead!=callbackWrite){retry=true;return true;}
        uint32_t source=longword(p),size=longword(p+4),rate=longword(p+8);
        if(!valid(source,size))return fail("AUDIO / EFFECT SOURCE EXTENT");
        if(longword(p+12) || longword(p+16))return fail("AUDIO / UNMEASURED EFFECT LOOP");
        PaulaSample::Layout layout;uint16_t period=0;uint32_t ticks=0;
        why=SoundEffect::describe((const uint8_t*)source,size,rate,0,0,layout,period,ticks,3546895);
        if(why)return fail(why);
        why=engine.effectOn((const uint8_t*)source,size,rate,word(p+24));if(why)return fail(why);
        int channel=engine.voices.allocatePaula();if(channel<0)return fail("AUDIO / EFFECT PAULA ALLOCATION");
        replaceChannel(channel);detach(channel);
        HalestormDma::Plan effectPlan;effectPlan.layout=layout;
        HalestormDma::convert(effectPlan,PaulaAudio::buffer(channel));
        engine.voices.effect.paula=channel;effectCallback=longword(p+30);effectContext=longword(p+34);
        if(!PaulaAudio::start(channel,layout,period,engine.voices.volume(127)))return fail(PaulaAudio::error());break;
    }
    case 18:
        if(word(p+24)==engine.effectID && engine.voices.effect.duration>=0){
            effectCallback=longword(p+30);effectContext=longword(p+34);
            if(!effectStop(true))return false;
        }break;
    case 19:
        if(argument>256)return fail("AUDIO / GAIN RANGE");engine.voices.gain=argument;
        for(const auto& voice:engine.voices.music)if(voice.paula>=0)PaulaAudio::volume(voice.paula,engine.voices.volume(voice.volume));
        if(engine.voices.effect.paula>=0)PaulaAudio::volume(engine.voices.effect.paula,engine.voices.volume(127));break;
    case 20:result=word(p+24)==engine.effectID && engine.voices.effect.duration>=0 ? 0 : 1;break;
    case 21:
        for(const auto& v:engine.voices.music)if(v.duration>=0)return fail("AUDIO / ACTIVE REINITIALIZATION");
        if(engine.voices.effect.duration>=0)return fail("AUDIO / ACTIVE REINITIALIZATION");
        why=engine.configure(word(p),word(p+2),word(p+4));musicCallback=musicContext=0;break;
    case 22:if(!effectStop(true))return false;break;
    case 24:why=engine.voices.setQuality(argument);break;
    case 27:musicCallback=longword(p);musicContext=longword(p+4);break;
    case 34:case 36:
        if(argument>=34)return fail("AUDIO / INSTRUMENT ID");engine.sounds[argument].loaded=selector==34;break;
    case 35:
        for(auto& v:engine.voices.music)stopVoice(v);engine.voices.silenceMusic();engine.voices.flags=0x7405;
        for(auto& sound:engine.sounds)sound.loaded=true;break;
    default:return fail("AUDIO / UNKNOWN SELECTOR");
    }
    if(why)return fail(why);return true;
}
#ifdef POP2_AUDIO_PROBE
void replay(bool enabled){replaying=enabled;}
bool advanceTo(uint32_t clock){
    if(!replaying || clock<engine.clock || clock-engine.clock>10000)return fail("AUDIO / REPLAY CLOCK");
    while(engine.clock<clock){engine.advance();if(!completed())return false;}return true;
}
#endif
}
#endif
