#include "mac/BlitterCompletion.h"
#include <cassert>
#include <cstdio>
// Model the asynchronous hardware boundary, not CPU timing. Complete a blit
// at every possible operation boundary, including immediately after start.
struct Hardware {
    static inline bool hasQueuedBlits=false,busy=false,irq=false,enabled=false;
    static inline unsigned queued=0,started=0,finished=0,clock=0,latency=0,due=0;
    static void tick() {++clock;if(busy && clock>=due){busy=false;irq=true;++finished;}}
    static void start() {busy=true;due=clock+latency;++started;tick();}
    static void setInterrupts(unsigned mask,bool enable){assert(mask==0x40);enabled=enable;tick();}
    static void clearInterruptRequests(unsigned mask){assert(mask==0x40);irq=false;tick();}
    static void processBlitterQueue(){
        tick();if(!busy && queued){--queued;hasQueuedBlits=queued!=0;start();}
    }
    static bool isBlitterBusy(){tick();return busy;}
    static void blitterDrain(){enabled=false;while(queued || busy)processBlitterQueue();}
    static void reset(unsigned count,unsigned delay){
        hasQueuedBlits=count!=0;queued=count;busy=false;irq=true;enabled=false;
        started=finished=clock=due=0;latency=delay;
    }
};
int main(){
    unsigned cases=0;
    for(unsigned count=0;count<=256;++count)for(unsigned delay=0;delay<=32;++delay){
        Hardware::reset(count,delay);
        BlitterCompletion::advance<Hardware>();
        for(unsigned step=0;step<12000 && (Hardware::queued || Hardware::busy || Hardware::irq);++step){
            Hardware::tick();
            if(Hardware::enabled && Hardware::irq)BlitterCompletion::advance<Hardware>();
            if(!Hardware::queued && !Hardware::busy)break;
        }
        assert(Hardware::finished==count && !Hardware::queued && !Hardware::busy);
        BlitterCompletion::drain<Hardware>();
        assert(!Hardware::enabled && !Hardware::irq);
        // CPU reuse/OS handback can arrive while any part of a batch is pending.
        Hardware::reset(count,delay);BlitterCompletion::advance<Hardware>();
        BlitterCompletion::drain<Hardware>();
        assert(Hardware::finished==count && !Hardware::queued && !Hardware::busy);
        assert(!Hardware::enabled && !Hardware::irq);++cases;
    }
    std::printf("BLITTER COMPLETION PASS: %u latency/batch cases and early drains\n",cases);
}
