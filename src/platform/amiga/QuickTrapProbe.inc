// Opt-in negative controls of the production indexed entries. Preserve all
// scheduler and heap state before continuing the ordinary integration route.
#ifdef POP2_QUICK_TRAP_PROBE
#ifndef POP2_VERIFY
#error Quick trap controls require verification
#endif
extern "C" {
uint32_t g_blockMoveChecks=0,g_quickWarmChecks=0;
uint32_t g_hunlockChecks=0,g_hunlockRejected=0,g_hunlockFreed=0;
uint32_t g_quickProbeCalls=0,g_quickProbeRejected=0,g_quickProbeFailures=0;
uint32_t g_quickDeadlineRejected=0,g_quickZoneRejected=0;
uint32_t g_quickCallbackChecks=0,g_quickCallbackRejected=0;
__attribute__((noinline)) void pop2QuickProbeDone(){__asm__ volatile("nop" ::: "memory");}
}
#include "QuickDisplayProbe.inc"
static void quickTrapFixture() {
    uint32_t unlockPC=0;
    const uint16_t traps[]={0xa9b4,0xa975,0xa055,0xa05d,0xa029,0xa069,0xa02a};
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
        if(trap==0xa02a)unlockPC=pc;
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
                trap==0xa029 ? pop2QuickHLockBody :
                trap==0xa02a ? pop2QuickHUnlockBody : pop2QuickHGetStateBody;
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
                        trap==0xa029 ? pop2QuickHLockWarm :
                        trap==0xa02a ? pop2QuickHUnlockWarm : pop2QuickHGetStateWarm;
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
    // Differential oracle for shared native warm bodies. Compare complete
    // saved registers/frame, live handle flags, both error locations and mode.
    // Checked C bodies retain the original-paired service semantics.
    auto dead=heap.newHandle(4,true);
    if(!dead || heap.disposeHandle(dead))++g_quickProbeFailures;
    auto compareWarm=[&](unsigned kind,MacHeap::Handle h,unsigned state,unsigned cc,
                        uint32_t input,unsigned oldMode) {
        uint32_t r[2][15];uint8_t f[2][8];
        int16_t errors[2];uint16_t memerrors[2];uint8_t modes[2],shadows[2],states[2];
        const char* reasons[2];uint32_t results[2];
        for(unsigned which=0;which<2;++which) {
            heap.setState(handle,state);*heap.errorStorage()=MacHeap::nilHandleErr;
            sw(low+0x220,0x1234);addressMode=oldMode;low[0xcb2]=oldMode;
            g_coreReason=reason;
            for(unsigned i=0;i<15;++i)r[which][i]=0x12340000+i;
            for(unsigned i=0;i<8;++i)f[which][i]=0x70+i;
            r[which][0]=input;r[which][8]=(uint32_t)h;sw(f[which],0x700|cc);
            auto body=kind==0 ? (which?pop2QuickHGetStateSlow:pop2QuickHGetStateWarm) :
                kind==1 ? (which?pop2QuickHLockSlow:pop2QuickHLockWarm) :
                          (which?pop2QuickSwapMMUModeSlow:pop2QuickSwapMMUModeWarm);
            results[which]=body(r[which],f[which]);
            errors[which]=heap.error();memerrors[which]=w(low+0x220);
            modes[which]=addressMode;shadows[which]=low[0xcb2];reasons[which]=g_coreReason;
            states[which]=heap.state(handle);
        }
        bool ok=results[0]==results[1] && errors[0]==errors[1] && memerrors[0]==memerrors[1] &&
            modes[0]==modes[1] && shadows[0]==shadows[1] && states[0]==states[1] && reasons[0]==reasons[1];
        for(unsigned i=0;i<15;++i)if(r[0][i]!=r[1][i])ok=false;
        for(unsigned i=0;i<8;++i)if(f[0][i]!=f[1][i])ok=false;
        if(ok)++g_quickWarmChecks;else ++g_quickProbeFailures;
    };
    for(unsigned kind=0;kind<2;++kind)for(unsigned state=0;state<256;state+=32)
        for(unsigned cc=0;cc<32;++cc)compareWarm(kind,handle,state,cc,0xdeadbeef,mode);
    for(unsigned old=0;old<2;++old)for(unsigned requested=0;requested<2;++requested)
        for(unsigned upper=0;upper<2;++upper)for(unsigned cc=0;cc<32;++cc)
            compareWarm(2,handle,0,cc,(upper?0xabcdef00:0)|requested,old);
    MacHeap::Handle invalidWarmHandles[]={nullptr,dead,(MacHeap::Handle)((uint8_t*)handle+1)};
    for(unsigned kind=0;kind<2;++kind)for(auto h:invalidWarmHandles)for(unsigned cc=0;cc<32;++cc)
        compareWarm(kind,h,0,cc,0xdeadbeef,mode);
    const uint32_t invalidModes[]={2,255,0xabcdef02,0xabcdefff};
    for(auto requested:invalidModes)for(unsigned cc=0;cc<32;++cc)
        compareWarm(2,handle,0,cc,requested,mode);
    g_coreReason=reason;addressMode=mode;low[0xcb2]=mode;
    // Compare the native service body with the full service across every
    // flag combination and CCR. The trap trampoline advances the fast PC.
    displayBatchFrame=displayBatchParent=displayBatchReturn=0;
    timerCount=vblCount=0;vblPass.pending=false;vblPass.active=false;
    if(unlockPC)for(unsigned state=0;state<256;state+=32)for(unsigned cc=0;cc<32;++cc) {
        uint32_t fast[15],full[15];uint8_t ff[8]={},sf[8]={},args[16]={};
        for(unsigned i=0;i<15;++i)fast[i]=full[i]=0x12340000+i;
        fast[8]=full[8]=(uint32_t)handle;
        sw(ff,cc);sw(sf,cc);sl(ff+2,unlockPC);sl(sf+2,unlockPC);
        heap.setState(handle,state);quickServicesValid=false;
        bool ok=pop2QuickHUnlockWarm(fast,ff)==1;
        ok=ok && heap.state(handle)==(state&0x7f) && w(low+0x220)==0;
        sl(ff+2,l(ff+2)+2);
        heap.setState(handle,state);
        ok=ok && pop2Dispatch(full,sf,args)==(uint32_t)args && !didStop;
        ok=ok && heap.state(handle)==(state&0x7f) && w(low+0x220)==0;
        for(unsigned i=0;i<15;++i)if(fast[i]!=full[i])ok=false;
        for(unsigned i=0;i<8;++i)if(ff[i]!=sf[i])ok=false;
        if(fast[0] || w(ff)!=((cc&16)|4))ok=false;
        if(ok)++g_hunlockChecks;else ++g_quickProbeFailures;
    }
    auto freed=heap.newHandle(16,true);
    if(!freed || heap.disposeHandle(freed))++g_quickProbeFailures;
    else if(unlockPC) {
        const uint32_t master=l((uint8_t*)freed);
        MacHeap::Handle bad[]={nullptr,freed,(MacHeap::Handle)((uint8_t*)handle+1)};
        for(auto h:bad)for(unsigned cc=0;cc<32;++cc) {
            uint32_t regs[15]={},before[15];uint8_t frame[8]={},saved[8],args[16]={};
            regs[0]=0x12345678;regs[8]=(uint32_t)h;sw(frame,cc);sl(frame+2,unlockPC);
            __builtin_memcpy(before,regs,sizeof(regs));__builtin_memcpy(saved,frame,8);
            const auto error=heap.error();const auto memerror=w(low+0x220);
            quickServicesValid=false;
            bool ok=pop2QuickHUnlockBody(regs,frame,args)==0;
            ok=ok && pop2QuickHUnlockWarm(regs,frame)==0;
            for(unsigned i=0;i<15;++i)if(regs[i]!=before[i])ok=false;
            for(unsigned i=0;i<8;++i)if(frame[i]!=saved[i])ok=false;
            if(heap.error()!=error || w(low+0x220)!=memerror || l((uint8_t*)freed)!=master)ok=false;
            if(ok)++g_hunlockRejected;else ++g_quickProbeFailures;
        }
        // Original-paired ending alias stays on its existing full-dispatch path.
        uint32_t pc=g_coreSegments[15][0]+0x29aa;
        if(!g_coreSegments[15][0] || !validMemory(pc,2) || w((uint8_t*)pc)!=0xa02a)++g_quickProbeFailures;
        else for(unsigned cc=0;cc<32;++cc) {
            uint32_t regs[15]={};uint8_t frame[8]={},args[16]={};
            regs[8]=(uint32_t)freed;sw(frame,cc);sl(frame+2,pc);
            quickServicesValid=false;
            bool ok=pop2QuickHUnlockBody(regs,frame,args)==0;
            ok=ok && pop2Dispatch(regs,frame,args)==(uint32_t)args && !didStop;
            ok=ok && regs[0]==uint32_t(int32_t(MacHeap::memWZErr)) && regs[8]==(uint32_t)freed;
            ok=ok && w(low+0x220)==uint16_t(MacHeap::memWZErr) && w(frame)==((cc&16)|8);
            ok=ok && l(frame+2)==pc+2 && l((uint8_t*)freed)==master;
            if(ok)++g_hunlockFreed;else ++g_quickProbeFailures;
        }
    }
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
    quickDisplayFixture();
    displayBatchFrame=owner;displayBatchParent=parent;displayBatchReturn=returned;
    timerCount=tasks;vblCount=vbls;vblPass=passes;timers[0]=first;
    addressMode=mode;low[0xcb2]=mode;quickServicesValid=false;
    if(heap.disposeHandle(handle))++g_quickProbeFailures;
    quickFixtureAudioPending=-1;
    g_coreReason=reason;normalExit=exitState;pop2QuickProbeDone();
}
#endif
