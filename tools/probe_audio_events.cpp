// Host service replay. Original calls and assets arrive only through stdin.
#include <cstdio>
#include <cstdlib>
#include <vector>
#include "mac/HalestormEngine.h"
static std::vector<uint8_t> inst[34],samples[34];
static bool lookup(uint32_t type,int16_t id,const uint8_t*& p,uint32_t& n){
    if(type==0x494e5354 && id>=0 && id<34){p=inst[id].data();n=inst[id].size();return true;}
    for(unsigned i=0;i<34;++i)if(type==0x736e6420 && id==HalestormInputs::word(inst[i].data())){p=samples[i].data();n=samples[i].size();return true;}
    return false;
}
static uint32_t number(){uint8_t b[4];if(fread(b,1,4,stdin)!=4)exit(2);return HalestormInputs::longword(b);}
static std::vector<uint8_t> bytes(unsigned n){std::vector<uint8_t> b(n);if(fread(b.data(),1,n,stdin)!=n)exit(2);return b;}
int main(){
    if(number()!=0x48415031)return 2;auto pitch=bytes(512);
    for(unsigned i=0;i<34;++i){inst[i]=bytes(22);samples[i]=bytes(number());}
    HalestormEngine engine;const char* why=engine.prepare(pitch.data(),lookup);if(why){fprintf(stderr,"%s\n",why);return 3;}
    unsigned count=number();
    for(unsigned i=0;i<count;++i){
        uint32_t mix=number(),selector=number(),argument=number();uint16_t packet[5];for(auto& v:packet)v=number();
        if(mix<engine.clock || mix-engine.clock>10000){fprintf(stderr,"mix %u / %u at %u\n",mix,engine.clock,i);return 4;}
        while(engine.clock<mix)engine.advance();engine.selected=-1;why=nullptr;
        switch(selector){
        case 6:engine.voices.silenceMusic();break;
        case 9:why=engine.noteOn(packet[0],packet[1],packet[2],packet[3],packet[4]);break;
        case 12:
            for(unsigned slot=0;slot<5;++slot){const auto& v=engine.voices.music[slot];if(v.duration>=0 && v.note==packet[1] && v.channel==packet[4] && !v.owner)printf("R %u %u %u\n",i,slot,engine.cursors[slot].position);}
            engine.voices.noteOff(packet[1],packet[4]);break;
        case 16:engine.clock=argument;break;
        case 21:why=engine.configure(packet[0],packet[1],packet[2]);engine.clock=mix;break;
        case 24:why=engine.voices.setQuality(argument);break;
        case 34:if(argument>=34)return 5;engine.sounds[argument].loaded=true;break;
        case 35:engine.voices.flags=0x7405;engine.voices.silenceMusic();for(auto& s:engine.sounds)s.loaded=true;break;
        case 36:if(argument>=34)return 5;engine.sounds[argument].loaded=false;break;
        }
        if(why){fprintf(stderr,"call %u selector %u: %s\n",i,selector,why);return 6;}
        if(selector==9){int slot=engine.selected;printf("N %u %d %u\n",i,slot,slot<0?0:engine.cursors[slot].step);}
    }
}
