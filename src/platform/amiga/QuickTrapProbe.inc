// Opt-in negative controls of the production indexed entries. Preserve all
// scheduler and heap state before continuing the ordinary integration route.
#ifdef POP2_QUICK_TRAP_PROBE
#ifndef POP2_VERIFY
#error Quick trap controls require verification
#endif
extern "C" {
uint32_t g_blockMoveChecks=0;
uint32_t g_quickProbeCalls=0,g_quickProbeRejected=0,g_quickProbeFailures=0;
uint32_t g_quickDeadlineRejected=0,g_quickZoneRejected=0;
uint32_t g_quickCallbackChecks=0,g_quickCallbackRejected=0;
__attribute__((noinline)) void pop2QuickProbeDone(){__asm__ volatile("nop" ::: "memory");}
}
static void quickTrapFixture() {
    const uint16_t traps[]={0xa9b4,0xa975,0xa055,0xa05d,0xa029,0xa069};
    uint32_t owner=displayBatchFrame,parent=displayBatchParent,returned=displayBatchReturn;
    unsigned tasks=timerCount,vbls=vblCount;TimerTask first=timers[0];VBLPasses passes=vblPass;
    const char* reason=g_coreReason;uint8_t mode=addressMode;bool exitState=normalExit;
    auto handle=heap.newHandle(16,true);
    if(!handle){++g_quickProbeFailures;pop2QuickProbeDone();return;}
    quickFixtureAudioPending=0;
    for(uint16_t trap:traps) {
        uint32_t pc=0;
        for(unsigned seg=0;seg<24 && !pc;++seg)
            for(uint32_t off=0;off+2<=g_coreSegments[seg][1];off+=2)
                if(w((uint8_t*)g_coreSegments[seg][0]+off)==trap){pc=g_coreSegments[seg][0]+off;break;}
        if(!pc){++g_quickProbeFailures;continue;}
        for(unsigned fault=0;fault<14;++fault) {
            uint32_t regs[15]={};regs[0]=0;regs[8]=(uint32_t)handle;
            uint8_t frame[8]={},args[32]={};sl(frame+2,pc);
            uint8_t* usp=args;
            displayBatchFrame=displayBatchParent=displayBatchReturn=0;
            timerCount=vblCount=0;quickServicesValid=false;
            if(fault==1)sw(frame,0x2000);
            if(fault==2)usp=stack+126;
            if(fault==3)usp=stack+65534;
            if(fault==4)sl(frame+2,0);
            if(fault==5)didStop=true;
            if(fault==6)g_timerCallbackActive=1;
            if(fault>=7 && fault<=9){
                displayBatchFrame=(uint32_t)(args+16);
                sl(args+16,0x12345678);sl(args+20,0x23456780);
                displayBatchParent=0x12345678;displayBatchReturn=0x23456780;
                if(fault==7)++displayBatchParent;
                if(fault==8)++displayBatchReturn;
                if(fault==9)usp=args+24;
            }
            if(fault==10)g_clockFault=1;
            if(fault==11){vblCount=1;vblPass.pending=1;}
            quickFixtureAudioPending=fault==13?1:0;
            if(fault==12){timerCount=1;timers[0]={args,{g_scaffoldFields-1,0},true};}
            uint32_t before[15];__builtin_memcpy(before,regs,sizeof(regs));
            uint32_t oldMode=addressMode;
            // The production table tags native register-image entries. Call
            // their C fallbacks here; the route separately audits native hits.
            auto entry=trap==0xa975 ? pop2QuickTickCountBody :
                trap==0xa9b4 ? pop2QuickSystemTaskBody :
                trap==0xa055 ? pop2QuickStripAddressBody :
                trap==0xa05d ? pop2QuickSwapMMUModeBody :
                trap==0xa029 ? pop2QuickHLockBody : pop2QuickHGetStateBody;
            uint32_t result=entry(regs,frame,usp);
            ++g_quickProbeCalls;
            if(!fault){
                // Positive controls reach every indexed body. SystemTask's
                // D0 is A5; the other selected zero inputs return zero.
                if(!result || regs[0]!=(trap==0xa9b4?(uint32_t)a5:trap==0xa069?0x80:trap==0xa05d?oldMode:0))++g_quickProbeFailures;
                for(unsigned i=1;i<15;++i)
                    if(regs[i]!=before[i])++g_quickProbeFailures;
                if(!(trap&0x0800)) {
                    // The native entries select these same warm bodies after
                    // their guard. Compare all saved registers and the complete
                    // exception frame with the checked C entry's result.
                    uint32_t warm[15];uint8_t warmFrame[8];
                    __builtin_memcpy(warm,before,sizeof(warm));
                    __builtin_memcpy(warmFrame,frame,sizeof(warmFrame));
                    sw(warmFrame,0);addressMode=oldMode;low[0xcb2]=oldMode;
                    auto body=trap==0xa055 ? pop2QuickStripAddressWarm :
                        trap==0xa05d ? pop2QuickSwapMMUModeWarm :
                        trap==0xa029 ? pop2QuickHLockWarm : pop2QuickHGetStateWarm;
                    bool same=body(warm,warmFrame)!=0;
                    for(unsigned i=0;i<15;++i)if(warm[i]!=regs[i])same=false;
                    for(unsigned i=0;i<8;++i)if(warmFrame[i]!=frame[i])same=false;
                    if(!same)++g_quickProbeFailures;
                }
                // A matching tick/field certificate must still reject a due
                // sub-field deadline, before touching any saved register.
                uint32_t untouched[15];__builtin_memcpy(untouched,regs,sizeof(regs));
                quickServicesTick=trap==0xa975?wallTicks():pop2ServiceTicks();
                quickServicesField=g_scaffoldFields;g_quickServicesMicro=0;
                timers[0]={args,{quickServicesField,0},true};timerCount=1;
                quickServicesValid=true;
                bool rejected=entry(regs,frame,usp)==0;
                for(unsigned i=0;i<15;++i)if(untouched[i]!=regs[i])rejected=false;
                if(rejected)++g_quickDeadlineRejected;else ++g_quickProbeFailures;
            } else {
                bool same=true;for(unsigned i=0;i<15;++i)if(before[i]!=regs[i])same=false;
                if(!result && same)++g_quickProbeRejected;else ++g_quickProbeFailures;
            }
            didStop=false;g_timerCallbackActive=0;g_clockFault=0;
            vblPass=passes;timers[0]=first;
        }
    }
    quickFixtureAudioPending=0;
    // Exercise the actual callback shortcut with all CCR inputs. No timer or
    // audio queues are changed: these two services only query/check the zone.
    uint32_t servicePC=g_coreServicePC,serviceSR=g_coreServiceSR,serviceActive=g_coreServiceActive;
    const uint16_t zones[]={0xa11a,0xa01b};
    for(uint16_t trap:zones) {
        uint32_t pc=0;
        for(unsigned seg=0;seg<24 && !pc;++seg)
            for(uint32_t off=0;off+2<=g_coreSegments[seg][1];off+=2)
                if(w((uint8_t*)g_coreSegments[seg][0]+off)==trap){pc=g_coreSegments[seg][0]+off;break;}
        if(!pc){++g_quickProbeFailures;continue;}
        for(unsigned ccr=0;ccr<32;++ccr) {
            uint32_t image[20],before[20];
            for(unsigned i=0;i<20;++i)image[i]=0x12340000+i;
            image[8]=trap==0xa01b?(uint32_t)arena:0x23456780;
            __builtin_memcpy(before,image,sizeof(image));
            g_coreServicePC=pc;g_coreServiceSR=ccr;g_timerCallbackActive=0;
            bool unchanged=callbackService(image)==0;
            for(unsigned i=0;i<20;++i)if(image[i]!=before[i])unchanged=false;
            if(unchanged)++g_quickCallbackRejected;else ++g_quickProbeFailures;
            g_timerCallbackActive=1;quickFixtureAudioPending=1;
            unchanged=callbackService(image)==0;
            for(unsigned i=0;i<20;++i)if(image[i]!=before[i])unchanged=false;
            if(unchanged)++g_quickCallbackRejected;else ++g_quickProbeFailures;
            quickFixtureAudioPending=0;
            bool good=callbackService(image)==(uint32_t)image && image[0]==0;
            for(unsigned i=1;i<15;++i)
                if(image[i]!=(i==8 && trap==0xa11a?(uint32_t)arena:before[i]))good=false;
            uint8_t* bytes=(uint8_t*)image;
            if(w(bytes+60)!=((ccr&~15)|4) || l(bytes+62)!=pc+2)good=false;
            for(unsigned i=66;i<sizeof(image);++i)if(bytes[i]!=((uint8_t*)before)[i])good=false;
            if(w(low+0x220)!=uint16_t(heap.error()))good=false;
            if(good)++g_quickCallbackChecks;else ++g_quickProbeFailures;
        }
    }
    g_coreServicePC=servicePC;g_coreServiceSR=serviceSR;g_coreServiceActive=serviceActive;g_timerCallbackActive=0;
    // An unsupported zone must fail before changing the saved registers.
    uint32_t invalid[15]={};invalid[0]=0x12345678;invalid[8]=(uint32_t)arena+4;
    bool rejectedZone=!trivialService<0xa01b>(invalid);
    const char expectedZone[]="HEAP / UNKNOWN ZONE";
    bool namedZone=true;
    for(unsigned i=0;i<sizeof(expectedZone);++i)
        if(g_coreReason[i]!=expectedZone[i]){namedZone=false;break;}
    if(rejectedZone &&
       invalid[0]==0x12345678 && invalid[8]==(uint32_t)arena+4 &&
       namedZone)
        ++g_quickZoneRejected;
    else ++g_quickProbeFailures;
    displayBatchFrame=owner;displayBatchParent=parent;displayBatchReturn=returned;
    timerCount=tasks;vblCount=vbls;vblPass=passes;timers[0]=first;
    addressMode=mode;low[0xcb2]=mode;quickServicesValid=false;
    if(heap.disposeHandle(handle))++g_quickProbeFailures;
    quickFixtureAudioPending=-1;
    g_coreReason=reason;normalExit=exitState;pop2QuickProbeDone();
}
#endif
