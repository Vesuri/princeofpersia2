#ifdef POP2_CORE
#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/memory.h>
#include "CoreRuntime.h"
#include "mac/BigEndianMemory.h"
#include "mac/CodeRangeCache.h"
#include "mac/VBLPasses.h"
#include "NativeFiles.h"
#include "NativeDisplay.h"
#include "NativeResources.h"
#ifdef POP2_AUDIO
#include "NativeAudio.h"
#include "PaulaAudio.h"
#ifdef POP2_AUDIO_PROBE
#include "../../../build/audio/AudioReplay.h"
#endif
#include "../../../build/audio/AudioInputs.h"
#endif
#ifdef POP2_UI
#include "NativeUI.h"
#include "NativeInput.h"
#include "../../../build/ui/UIInputs.h"
#endif
#include "framework/AmigaHardware.h"
#include "mac/MacHeap.h"
#include "mac/OriginalApplication.h"
#include "mac/SHA256.h"
#include "mac/Identity.h"
#include "mac/FixedMath.h"
#include "../../../build/core/Inputs.h"
#ifdef POP2_DISPLAY
#include "../../../build/display/VideoTransfer.h"
#ifdef POP2_DISPLAY_FRAMES_PROBE
#include "../../../build/display/DisplayFrames.h"
#endif
#endif
#ifdef POP2_FILE_REPLAY
#include "../../../build/core/FileReplay.h"
#endif
extern DosLibrary* DOSBase;
extern "C" {
void pop2_call_mac(void*,void*,void*);
void pop2_timer_callback();
uint32_t pop2WallTicks();
extern volatile uint32_t g_clockFault;
#ifdef POP2_INTEGRATION_PROBE
void pop2_integration_seed(void*,void*);void pop2_integration_title();
uint32_t g_integrationOpeningReturn=0,g_integrationTitleTarget=0,g_integrationRoomReturn=0,g_integrationDeathTarget=0,g_integrationDeathReturn=0,g_integrationDeathKind=0;
volatile uint32_t g_integrationStage=0,g_integrationRightAt=0;
volatile uint32_t g_integrationTitlePhase=0,g_integrationTitleCue=0;
void pop2_integration_opening();void pop2_integration_room();void pop2_integration_death();void pop2_integration_death_return();
extern volatile uint32_t g_integrationPulseRaw,g_integrationPulseStart,g_integrationPulseEnd;
#ifdef POP2_TIMING_PROBE
volatile uint32_t g_profileFreeFast=0,g_profileLargestFast=0,g_profileFreeChip=0;
#endif
__attribute__((noinline)) void pop2IntegrationStage(){__asm__ volatile("nop" ::: "memory");}
static void recordIntegrationStage(){
#ifdef POP2_TIMING_PROBE
    // Snapshots at checkpoints, outside the measured gameplay window.
    g_profileFreeFast=AvailMem(MEMF_FAST);
    g_profileLargestFast=AvailMem(MEMF_FAST|MEMF_LARGEST);
    g_profileFreeChip=AvailMem(MEMF_CHIP);
#endif
    pop2IntegrationStage();
}
#endif
#ifdef POP2_GAMEPLAY_RESTART_PROBE
#if !defined(POP2_VERIFY) || !defined(POP2_INTEGRATION_PROBE)
#error Gameplay restart probe requires explicit verification and integration input
#endif
volatile uint32_t g_gameplayRooms=0,g_gameplayAudioSelector=0,g_gameplayMenu=0;
volatile uint32_t g_gameplaySuspendCalls=0,g_gameplayResumeCalls=0,g_gameplaySuspendClock=0,g_gameplayResumeClock=0;
#endif
#ifdef POP2_GAMEPLAY_SAVE_PROBE
#ifndef POP2_GAMEPLAY_RESTART_PROBE
#error Save probe requires the verified gameplay harness
#endif
uint32_t g_gameplaySaveReturn=0,g_gameplayLoadReturn=0;
volatile uint32_t g_gameplaySaveResult=0,g_gameplayLoadResult=0,g_gameplaySavedAt=0,g_gameplayLoadedFrames=0;
volatile uint32_t g_gameplayInputActions=0,g_gameplayKeyCount=0,g_gameplayKeys[128][4]={};
void pop2_gameplay_saved();void pop2_gameplay_loaded();
__attribute__((noinline)) void pop2GameplayFileCheckpoint(){__asm__ volatile("nop" ::: "memory");}
#endif
#ifdef POP2_TIMING_TM_PROBE
#if !defined(POP2_VERIFY) || !defined(POP2_INTEGRATION_PROBE) || defined(POP2_TIMING_VBL_PROBE)
#error Time Manager fixture requires verification and a separate original-opening run
#endif
void pop2_timing_tm_fixture();void pop2_timing_tm_callback();void pop2_timing_tm_end();
uint32_t g_timingTmRecords=0;
volatile uint32_t g_timingTmResult=0,g_timingTmUserCalls=0;
#endif
#ifdef POP2_TIMING_VBL_PROBE
#if !defined(POP2_VERIFY) || !defined(POP2_INTEGRATION_PROBE)
#error Timing VBL fixture requires verification and the original opening hook
#endif
void pop2_timing_vbl_fixture();void pop2_timing_vbl_callback();void pop2_timing_vbl_end();
uint32_t g_timingVblRecords=0;
volatile uint32_t g_timingVblResult=0,g_timingVblElapsed=0,g_timingVblUserCalls=0,g_timingVblDeferredFields=0;
#endif
#ifdef POP2_TIMING_PROBE
volatile uint32_t g_timingCount=0,g_timingRows[256][8]={};
volatile uint32_t g_timingSetCount=0,g_timingSetRows[512][5]={};
__attribute__((noinline)) void pop2TimingWindowComplete(){__asm__ volatile("nop" ::: "memory");}
volatile uint32_t g_timingPhase=0x1000,g_timingSamples[0x1005]={};
#endif
uint32_t g_timerCallbackCode=0,g_timerCallbackTask=0,g_timerCallbackActive=0,g_timerCallbacks=0;
void pop2_audio_callback();void pop2_audio_lookup();void pop2_audio_invoke();void pop2_audio_fixture_end();
uint32_t g_audioCallbackCode=0,g_audioCallbackContext=0;
void pop2_audio_test_callback();
uint32_t g_audioBefore[13]={},g_audioAfter[13]={};
uint32_t g_audioCalls=0,g_audioDone=0,g_audioCallbacks=0,g_audioABI=0;
uint32_t g_audioRows[16000][6]={};
uint32_t g_audioLookupType=0x4a6e7468;
uint32_t g_audioEntry=0,g_audioArgument=0,g_audioSelector=0,g_audioResult=0;
void pop2_unknown_fixture();void pop2_illegal_fixture();
void pop2_identity_fixture();void pop2_identity_fixture_end();
void pop2_file_fixture();void pop2_file_fixture_end();
void pop2_replay_fixture();void pop2_replay_fixture_end();
uint32_t g_replayBegin=0,g_replaySize=0,g_replayCount=0,g_replayCompleted=0;uint16_t g_replayRefs[256]={};uint32_t g_replayInfoDates[2]={};
void pop2_display_fixture();void pop2_display_fixture_end();
uint32_t g_displayServiceProbe=0;
uint32_t g_displayWindow=0,g_displayPalette=0,g_displaySceneColors=0,g_displayScene=0,g_displayLogicalScreen=0;
void pop2_display_rejection_fixture();
void pop2_display_scene_init();void pop2_display_scene_frame();void pop2_display_scene_end();
extern volatile uint16_t g_displayParity;
__attribute__((noinline)) void pop2DisplayOddFieldComplete() { __asm__ volatile("nop" ::: "memory"); }
__attribute__((noinline)) void pop2DisplaySceneComplete() { __asm__ volatile("nop" ::: "memory"); }
extern volatile uint32_t g_scaffoldFields;
__attribute__((noinline)) void pop2DisplayServicesComplete() { __asm__ volatile("nop" ::: "memory"); }
void pop2_resource_fixture();void pop2_resource_fixture_end();
uint32_t g_specProbeCount=0,g_specProbePaths[11]={},g_specProbeOutputs[11]={};int16_t g_specProbeResults[11]={};
uint32_t g_resourceProbeNames[4]={},g_resourceProbeHandle=0,g_resourceProbeOption=0,g_resourceProbeInfo=0;
uint16_t g_resourceProbeRefs[4]={};uint32_t g_resourceProbeSizes[2]={};
uint32_t g_fileProbePB=0,g_fileProbeBuffer=0,g_fileProbeResults[3]={0xffffffff,0xffffffff,0xffffffff};
uint32_t g_fileProbeWriteMode=0;int32_t g_fileProbeWriteResult=-1;
uint32_t g_fileProbeFCB=0;int32_t g_fileProbeFCBResults[3]={-1,-1,-1};
uint32_t g_coreIdentity[7][3]={},g_coreSysEnvError=0;
uint8_t g_coreSysEnv[16]={};
void pop2_ui_fixture();void pop2_ui_fixture_end();
uint32_t g_uiReport[31]={},g_uiText=0,g_uiWindow=0;
#ifdef POP2_UI_BAD_ABOUT
uint16_t g_uiProbeAboutID=601;
#else
uint16_t g_uiProbeAboutID=600;
#endif
volatile uint8_t g_uiProbeReplyRaw=0x44;
__attribute__((noinline)) void pop2UIFixtureComplete(){__asm__ volatile("nop" ::: "memory");}
void pop2_cached_services();void pop2_cached_services_end();
void pop2_line_a();void pop2_mac_exit();void pop2_old_loadseg();void pop2_old_exit();
uint32_t g_coreException=0,g_coreActiveFrame=0,g_coreFaultPC=0;
uint32_t g_coreServicePC=0;uint16_t g_coreServiceSR=0,g_coreServiceActive=0;
void pop2_exception_2();
void pop2_exception_3();
void pop2_exception_4();
void pop2_exception_5();
void pop2_exception_6();
void pop2_exception_7();
void pop2_exception_8();
void pop2_exception_9();
void pop2_exception_11();
void pop2_exception_14();
uint32_t g_coreResources[32][3]={},g_coreResourceCount=0,g_coreStrs=0;
#ifdef POP2_UI
constexpr uint32_t traceCapacity=4096;
#else
constexpr uint32_t traceCapacity=128;
#endif
uint32_t g_coreHostSP=0,g_coreTrace[traceCapacity][3]={},g_coreCount=0,g_coreA5=0,g_coreBelow=0;
uint32_t g_coreSegments[24][2]={},g_coreInitial=0,g_coreStopTrap=0,g_coreStopSegment=0,g_coreStopOffset=0;
uint32_t g_coreZoneBytes=3*1024*1024,g_coreHeapOK=0,g_coreVectorRestored=0,g_coreCodeAudit=0;
const char* g_coreReason="CORE / NOT STARTED";
volatile uint32_t g_coreLoadedSegment=0;
__attribute__((noinline)) void pop2CoreSegmentLoaded(){__asm__ volatile("nop" ::: "memory");}
__attribute__((noinline)) void pop2CoreStopped() { __asm__ volatile("nop" ::: "memory"); }
}
namespace {
uint16_t w(const uint8_t* p) { return BigEndianMemory::read16(p); }
uint32_t l(const uint8_t* p) { return BigEndianMemory::read32(p); }
void sw(uint8_t* p,uint16_t v) { BigEndianMemory::write16(p,v); }
void sl(uint8_t* p,uint32_t v) { BigEndianMemory::write32(p,v); }
void copy(uint8_t* d,const uint8_t* s,uint32_t n) {
    // Keep byte destinations explicit: the m68k compiler can combine the
    // advancing source and indexed destination into a shifted MOVE.B.
    volatile uint8_t* destination=d;
    for(uint32_t i=0;i<n;++i)destination[i]=s[i];
}
OriginalApplication app;MacHeap heap;
volatile uint32_t* vectorTable=0;uint32_t savedVectors[11],portVectors[11];
const uint16_t vectorIndices[]={2,3,4,5,6,7,8,9,10,11,14};
bool vectorsInstalled=false;
void flushCode() {
    // Exec's cache service uses its privilege-vector machinery in user mode.
    // Give that verified OS call its own exception vectors, then restore ours.
    bool suspended=CoreRuntime::suspendVectors();
    CacheClearU();
    if(suspended)CoreRuntime::resumeVectors();
}
uint8_t *file=0,*plan=0,*arena=0,*world=0,*stack=0,*initial=0,*a5=0,*low=0;
uint32_t fileSize=0,planSize=0,worldSize=0;
uint8_t* audioPitch=nullptr;uint32_t audioPitchSize=0;
#ifdef POP2_AUDIO_PROBE
uint8_t* audioReplay=nullptr;uint32_t audioReplaySize=0;uint8_t* audioPacket=nullptr;
#endif
uint8_t* uiPlan=nullptr;uint32_t uiPlanSize=0;
uint8_t* displayFrames=nullptr;uint32_t displayFramesSize=0;
uint8_t* videoTransfer=nullptr;uint32_t videoTransferSize=0;
bool didStop=false;
uint8_t auditBuffer[65536];
uint8_t* patchedLoad=0;uint8_t* patchedExit=0;
struct Resource { uint32_t type;int16_t id;OriginalApplication::Resource source;MacHeap::Handle handle;bool detached;uint32_t identity;int16_t fileRef; };
Resource resources[1024]={};unsigned resourceCount=0;int16_t resourceError=0;
bool validMemory(uint32_t at,uint32_t n) {
    return (at>=(uint32_t)stack && at-(uint32_t)stack<=65536 && n<=65536-(at-(uint32_t)stack)) ||
        (at>=(uint32_t)world && at-(uint32_t)world<=worldSize && n<=worldSize-(at-(uint32_t)world)) ||
        (at>=(uint32_t)arena && at-(uint32_t)arena<=g_coreZoneBytes && n<=g_coreZoneBytes-(at-(uint32_t)arena));
}
bool fail(const char* why);
// Time Manager deadlines use PAL fields and beam position; callbacks run only
// from the user-mode service return, with A1 pointing at the original TMTask.
struct TimerStamp { uint32_t field,micro; };
struct TimerTask { uint8_t* task;TimerStamp due;bool active; };
TimerTask timers[32]={};
unsigned timerCount=0;
uint32_t soundVolume=0x006e006e;
uint8_t addressMode=0;
uint8_t* vblTasks[8]={};unsigned vblCount=0;
VBLPasses vblPass;
uint32_t displayBatchFrame=0,displayBatchParent=0,displayBatchReturn=0;
bool displayBatchScene=false;
uint32_t callbackKind=0; // 1: Time Manager, 2: virtual VBL pass.
uint32_t wallTicks() {
#ifdef POP2_INPUT
    return pop2WallTicks();
#else
    return g_scaffoldFields*6/5;
#endif
}
uint32_t callbackTicks() {return g_timerCallbackActive && callbackKind==2 ? vblPass.tick : wallTicks();}
int vblIndex(uint8_t* task) {
    for(unsigned i=0;i<vblCount;++i)if(vblTasks[i]==task)return i;
    return -1;
}
void removeVBL(unsigned index) {
    if(index)sl(vblTasks[index-1],index+1<vblCount?(uint32_t)vblTasks[index+1]:0);
    vblPass.removed(index);
    for(unsigned i=index+1;i<vblCount;++i)vblTasks[i-1]=vblTasks[i];
    vblTasks[--vblCount]=nullptr;
    if(!vblCount)vblPass.reset(wallTicks());
}
TimerStamp timerNow() {
    uint32_t before,after;uint16_t hi,lo,check;
    do { before=g_scaffoldFields;hi=*vposrPointer;lo=*vhposrPointer;
         check=*vposrPointer;after=g_scaffoldFields;
    } while(before!=after || ((hi^check)&1));
    uint32_t line=((hi&1)<<8)|(lo>>8);
    uint32_t us=line*64+((lo&255)*64)/227;
    return {after,us<20000?us:19999};
}
bool timerDue(TimerStamp now,TimerStamp due) {
    int32_t fields=now.field-due.field;
    return fields>0 || (!fields && now.micro>=due.micro);
}
bool timerService(uint16_t trap,uint32_t* regs) {
    uint8_t* task=(uint8_t*)regs[8];
    if(!validMemory(regs[8],22))return fail("TIME / TASK EXTENT");
    TimerTask* slot=nullptr;TimerTask* empty=nullptr;
    for(unsigned i=0;i<timerCount;++i){
        auto& t=timers[i];if(t.task==task){slot=&t;break;}
        if(!t.task && !empty)empty=&t;
    }
    if(trap==0xa058) {
        if(slot)return fail("TIME / DUPLICATE INSTALL");
        if(!empty && timerCount<32)empty=&timers[timerCount++];
        if(!empty)return fail("TIME / TASK CAPACITY");
        *empty={task,{0,0},false};sl(task,0);sw(task+4,w(task+4)&0x1fff);
    } else if(trap==0xa059) {
        uint32_t remaining=0;
        if(slot && slot->active){
            TimerStamp now=timerNow();
            if(!timerDue(now,slot->due)){
                uint32_t fields=slot->due.field-now.field;
                int32_t micro=slot->due.micro-now.micro;
                if(micro<0){--fields;micro+=20000;}
                if(fields<107374 || (fields==107374 && uint32_t(micro)<=3648))
                    remaining=0-(fields*20000+micro);
                else remaining=fields*20+(micro+999)/1000;
            }
        }
        sl(task+10,remaining);sw(task+4,w(task+4)&0x7fff);
        if(slot)*slot={};
        while(timerCount && !timers[timerCount-1].task)--timerCount;
    } else {
        if(!slot)return fail("TIME / PRIME UNINSTALLED TASK");
        // MEASURED System 7.5.5: priming an active task succeeds without
        // replacing its deadline. The original MIDI callback does this twice.
        if(slot->active){regs[0]=0;return true;}
        TimerStamp now=timerNow();int32_t delay=regs[0];uint32_t magnitude=delay<0?0-regs[0]:regs[0];
        uint32_t fields=delay<0?magnitude/20000:magnitude/20;
        uint32_t micro=delay<0?magnitude%20000:(magnitude%20)*1000;
        micro+=now.micro;if(micro>=20000){++fields;micro-=20000;}
        slot->due={now.field+fields,micro};slot->active=true;
        sw(task+4,w(task+4)|0x8000);
    }
    regs[0]=0;return true;
}
bool fixedService(uint16_t trap,uint8_t* args) {
    if(!validMemory((uint32_t)args,12))return fail("FIXED / ARGUMENT EXTENT");
    sl(args+8,trap==0xa84d?FixedMath::divide(int32_t(l(args+4)),int32_t(l(args))):FixedMath::multiply(int32_t(l(args+4)),int32_t(l(args))));
    return true;
}
bool pascalPath(uint32_t address,char path[256]) {
    if(!validMemory(address,1))return false;uint8_t* p=(uint8_t*)address;
    if(!validMemory(address+1,p[0]))return false;
    for(unsigned i=0;i<p[0];++i)path[i]=p[i+1];path[p[0]]=0;return true;
}
bool fail(const char* why) { g_coreReason=why;return false; }
bool readFile(const char* name,uint8_t*& data,uint32_t& size,uint32_t limit,const uint8_t digest[32]) {
    BPTR f=Open((CONST_STRPTR)name,MODE_OLDFILE);if(!f)return fail("CORE INPUT / OPEN");
    LONG seek=Seek(f,0,OFFSET_END);LONG n=seek<0 ? -1 : Seek(f,0,OFFSET_BEGINNING);
    if(n<=0 || uint32_t(n)>limit) { Close(f);return fail("CORE INPUT / SIZE"); }
    size=n;data=(uint8_t*)AllocMem(size,MEMF_PUBLIC);
    if(!data) { Close(f);return fail("CORE INPUT / MEMORY"); }
    LONG got=Read(f,data,size);Close(f);if(got!=(LONG)size)return fail("CORE INPUT / READ");
#ifdef POP2_VERIFY
    SHA256 sha;uint8_t actual[32];sha_init(&sha);sha_update(&sha,data,size);sha_final(&sha,actual);
    for(unsigned i=0;i<32;++i)if(actual[i]!=digest[i])return fail("CORE INPUT / SHA256");
#endif
    return true;
}
bool relocate(int16_t segment,uint8_t* code,uint32_t size) {
    for(uint32_t i=0;i<l(plan);++i) {
        const uint8_t* p=plan+4+32*i;if(w(p)!=segment)continue;
        uint32_t n=w(p+2),at=l(p+4);
        if(!n || n>12 || at>size || n>size-at)return fail("LOW MEMORY / PATCH EXTENT");
        for(uint32_t j=0;j<n;++j)if(code[at+j]!=p[8+j])return fail("LOW MEMORY / ORIGINAL BYTE MISMATCH");
        copy(code+at,p+20,n);
    }
#ifdef POP2_UI
    for(uint32_t i=0;i<l(uiPlan);++i){
        const uint8_t* p=uiPlan+4+32*i;if(w(p)!=segment)continue;
        uint32_t n=w(p+2),at=l(p+4);
        if(!n||n>12||at>size||n>size-at)return fail("UI / PATCH EXTENT");
        for(uint32_t j=0;j<n;++j)if(code[at+j]!=p[8+j])return fail("UI / ORIGINAL BYTE MISMATCH");
        copy(code+at,p+20,n);
    }
#endif
    flushCode();return true;
}
Resource* byHandle(MacHeap::Handle h) {
    for(unsigned i=0;i<resourceCount;++i)if(resources[i].handle==h)return resources+i;
    return 0;
}
bool load(Resource& r) {
    if(*r.handle)return true;
    if(heap.reallocateHandle(r.handle,r.source.size))return fail("RESOURCE / ALLOCATION");
    int32_t readError=NativeResources::read(r.identity,*r.handle,r.source.size);
    if(readError)return fail("RESOURCE / PAYLOAD READ");
    if(r.type==0x4a6e7468)flushCode();
    if(r.type==0x53545253)g_coreStrs=(uint32_t)*r.handle;
#ifdef POP2_BAD_RELOCATION
    if(r.type==0x434f4445 && r.id==1)(*r.handle)[16]^=1;
#endif
#ifdef POP2_BAD_UI_GUARD
    if(r.type==0x434f4445 && r.id==2)(*r.handle)[0x1a40]^=1;
#endif
    if(r.type==0x434f4445 && !relocate(r.id,*r.handle,r.source.size))return false;
    heap.setState(r.handle,0x20|((r.source.attributes&0x10)?0x80:0)|((r.source.attributes&0x20)?0x40:0));
    return true;
}
Resource* get(uint32_t type,int16_t id) {
#ifdef POP2_AUDIO
    if(type==0x4d445256){resourceError=0;fail("AUDIO / ORIGINAL MDRV FORBIDDEN");return 0;}
#endif
    NativeResources::Item item={};resourceError=NativeResources::find(type,id,false,item);
    sw(low+0xa60,resourceError);
    if(resourceError)return 0;
    for(unsigned i=0;i<resourceCount;++i)if(resources[i].identity==item.identity && resources[i].handle && !resources[i].detached) {
        if(low[0xa5e] && !load(resources[i]))return 0;
        return resources+i;
    }
    unsigned index=0;while(index<resourceCount && resources[index].handle)++index;
    if(index==1024) { fail("RESOURCE / SLOT CAPACITY");return 0; }
    Resource& r=resources[index];r.type=type;r.id=id;r.detached=false;r.identity=item.identity;r.fileRef=item.file;
    r.source={0,item.size,item.attributes};
    r.handle=heap.newEmptyHandle();if(!r.handle) { fail("RESOURCE / MASTER POINTER");return 0; }
    if(index==resourceCount)++resourceCount;heap.setState(r.handle,0x20);
    if(low[0xa5e] && !load(r))return 0;
    return &r;
}
bool segment(int16_t id) {
    if(id<1 || id>=24)return fail("LOADSEG / SEGMENT ID");
    Resource* r=get(0x434f4445,id);
    if(!r)return resourceError ? fail("LOADSEG / RESOURCE ERROR") : false;
    if(!load(*r))return false;
    uint8_t* code=*r->handle;uint32_t size=r->source.size;
    heap.setState(r->handle,heap.state(r->handle)|0x80);
    uint32_t first=w(code),count=w(code+2);auto spec=app.world();
    if(size<4 || (first&0x8007) || first>spec.jumpBytes || count>(spec.jumpBytes-first)/8)return fail("LOADSEG / NEAR HEADER");
    for(uint32_t i=0;i<count;++i) {
        uint8_t* entry=a5+spec.jumpOffset+first+8*i;
        if(w(entry+2)==0x4ef9 && w(entry)==id)continue;
        if(w(entry+2)!=0x3f3c || w(entry+4)!=id || w(entry+6)!=0xa9f0 || w(entry)>=size-4)return fail("LOADSEG / JUMP ENTRY");
        uint32_t target=(uint32_t)code+4+w(entry);
#ifdef POP2_INTEGRATION_PROBE
        // Test-only call wrappers: the original routines still execute, with
        // the same explicit seed argument as the Macintosh reference.
        if(id==15 && target==(uint32_t)code+0x6fec){g_integrationTitleTarget=target;target=(uint32_t)pop2_integration_title;}
        if(id==5 && target==(uint32_t)code+0x4afc){g_integrationDeathTarget=target;target=(uint32_t)pop2_integration_death;}
#endif
        sw(entry,id);sw(entry+2,0x4ef9);sl(entry+4,target);
    }
    g_coreSegments[id][0]=(uint32_t)code;g_coreSegments[id][1]=size;
#ifdef POP2_INTEGRATION_PROBE
    if(id==17){flushCode();pop2_integration_seed(code+0x6e42,a5);}

#endif
    flushCode();g_coreLoadedSegment=id;pop2CoreSegmentLoaded();return true;
}
bool auditCode() {
    for(unsigned id=1;id<24;++id)if(g_coreSegments[id][0]) {
        OriginalApplication::Resource original;
        if(!app.resource(0x434f4445,id,original) || original.size>sizeof auditBuffer)return fail("CODE / AUDIT EXTENT");
        copy(auditBuffer,original.bytes,original.size);
        if(!relocate(id,auditBuffer,original.size))return false;
        if(id==1)sl(auditBuffer+4,g_coreStrs);
        if(id==19)sl(auditBuffer+0x478,(uint32_t)a5);
        const uint8_t* actual=(const uint8_t*)g_coreSegments[id][0];
        for(uint32_t i=0;i<original.size;++i)if(actual[i]!=auditBuffer[i])return fail("CODE / UNEXPECTED WRITE");
    }
    return true;
}
CodeRangeCache codeRanges;
bool location(uint32_t pc,uint32_t& seg,uint32_t& offset) {
    return codeRanges.find(g_coreSegments,24,pc,seg,offset);
}
uint32_t stop(uint8_t* frame,uint8_t* usp,const char* why,uint16_t trap,uint32_t seg,uint32_t off) {
    didStop=true;g_coreReason=why;g_coreStopTrap=trap;g_coreStopSegment=seg;g_coreStopOffset=off;
    sl(frame+2,(uint32_t)pop2_mac_exit);return (uint32_t)usp;
}
}
namespace CoreRuntime {
const char* error() { return g_coreReason; }
const uint8_t* displayTransfer() {return videoTransfer;}
bool prepare() {
    if(!readFile("Prince of Persia 2.rsrc",file,fileSize,1024*1024,applicationSHA256))return false;
    if(!app.open(file,fileSize,applicationSHA256))return fail(app.error());
    if(!NativeFiles::prepare())return fail("FILES / INSTALLED CATALOG OR HASH");
    if(!NativeResources::prepare(file,fileSize))return fail("RESOURCE / APPLICATION DIRECTORY");
    if(!readFile("relocations.bin",plan,planSize,65536,relocationsSHA256))return false;
#ifdef POP2_UI
    if(!readFile("ui-seams.bin",uiPlan,uiPlanSize,65536,uiSeamsSHA256))return false;
    if(uiPlanSize<4||l(uiPlan)>(uiPlanSize-4)/32||4+l(uiPlan)*32!=uiPlanSize)return fail("UI / PLAN EXTENT");
#endif
    if(planSize<4 || l(plan)>(65536-4)/32 || planSize!=4+32*l(plan))return fail("LOW MEMORY / PLAN SIZE");
    auto spec=app.world();if(spec.above>8192)return fail("A5 / SHADOW OVERLAP");
    worldSize=spec.below+8192+8192;
    arena=(uint8_t*)AllocMem(g_coreZoneBytes,MEMF_PUBLIC|MEMF_CLEAR);
    world=(uint8_t*)AllocMem(worldSize,MEMF_PUBLIC|MEMF_CLEAR);
    stack=(uint8_t*)AllocMem(65536,MEMF_PUBLIC|MEMF_CLEAR);
#ifdef POP2_VERIFY
    initial=(uint8_t*)AllocMem(spec.below,MEMF_PUBLIC|MEMF_CLEAR);
    if(!initial)return fail("CORE / MEMORY");
#endif
    if(!arena || !world || !stack)return fail("CORE / MEMORY");
    if(!heap.init(arena,g_coreZoneBytes))return fail("CORE / APPLICATION ZONE");
    a5=world+spec.below;low=a5+8192;g_coreA5=(uint32_t)a5;g_coreBelow=spec.below;g_coreInitial=(uint32_t)initial;
    sl(low+0x130,(uint32_t)arena+g_coreZoneBytes);sl(low+0x118,(uint32_t)arena);sl(low+0x2aa,(uint32_t)arena);
    auto launchParameters=heap.newHandle(4,true);
    if(!launchParameters)return fail("CORE / LAUNCH PARAMETERS");
    sl(low+0xaec,(uint32_t)launchParameters); // appOpen, zero Finder documents.
    // Public drive/VCB queue and unit-table records for the one native volume.
    // The game reads these directly to classify fixed, writable storage.
    uint8_t* drive=heap.newPtr(24,true);uint8_t* vcb=heap.newPtr(178,true);
    auto dce=heap.newHandle(40,true);uint8_t* driver=heap.newPtr(32,true);
    uint8_t* units=heap.newPtr(4,true);
    if(!drive || !vcb || !dce || !driver || !units)return fail("CORE / VOLUME RECORDS");
    sl(drive,0x00080100);sw(drive+8,1);sw(drive+10,1);sw(drive+12,0xffff);
    sl(low+0x30a,(uint32_t)drive+4);sl(low+0x30e,(uint32_t)drive+4);
    sl(low+0x358,(uint32_t)vcb);sl(low+0x35c,(uint32_t)vcb);sw(vcb+78,0xffff);
    sl(low+0x1000,(uint32_t)low+0x308);sl(low+0x1004,(uint32_t)low+0x356);
    sl(*dce,(uint32_t)driver);sw(*dce+4,0x2000);sw(*dce+24,0xffff);
    const char* driverName=".Native";driver[18]=7;for(unsigned i=0;i<7;++i)driver[19+i]=driverName[i];
    sl(units,(uint32_t)dce);sl(low+0x11c,(uint32_t)units);
    low[0xa5e]=0xff;sl(low+0x904,(uint32_t)a5);sl(low+0x908,(uint32_t)world);
    sw(low+0x900,NativeResources::current());sw(low+0xa5a,NativeResources::current());
    sw(low+0x28e,0x3fff);low[0x12f]=3;sw(low+0x15a,0x0755);
    OriginalApplication::Resource code0;app.resource(0x434f4445,0,code0);
    copy(a5+spec.jumpOffset,code0.bytes+16,spec.jumpBytes);
#ifdef POP2_DISPLAY
    if(!readFile("video-transfer.bin",videoTransfer,videoTransferSize,256,videoTransferSHA256))return false;
    if(videoTransferSize!=256)return fail("DISPLAY / VIDEO TRANSFER INPUT");
    if(!NativeDisplay::prepare(heap,low,validMemory,[](uint32_t type,int16_t id,const uint8_t*& bytes,uint32_t& size) {Resource* r=get(type,id);if(!r || !load(*r))return false;bytes=*r->handle;size=heap.handleSize(r->handle);return true;}))return fail(NativeDisplay::error());
#endif
#ifdef POP2_UI
    if(!NativeUI::prepare(heap,low,validMemory,[](uint32_t type,int16_t id,const uint8_t*& bytes,uint32_t& size){Resource* r=get(type,id);if(!r || !load(*r))return false;bytes=*r->handle;size=heap.handleSize(r->handle);return true;}))return fail(NativeUI::error());
#endif
#ifdef POP2_AUDIO
    if(!readFile("audio-pitch.bin",audioPitch,audioPitchSize,512,audioPitchSHA256))return false;
    if(audioPitchSize!=512)return fail("AUDIO / PITCH INPUT SIZE");
#ifdef POP2_AUDIO_PROBE
    if(!readFile("audio-replay.bin",audioReplay,audioReplaySize,2097152,audioReplaySHA256))return false;
    if(audioReplaySize<12 || l(audioReplay)!=0x50415231 || l(audioReplay+4)>16000 || l(audioReplay+8)>1048576 || audioReplaySize!=12+l(audioReplay+4)*64+l(audioReplay+8))return fail("AUDIO / REPLAY EXTENT");
    uint8_t* ownedReplay=heap.newPtr(audioReplaySize,true);audioPacket=heap.newPtr(128,true);
    if(!ownedReplay || !audioPacket)return fail("AUDIO / REPLAY MEMORY");
    copy(ownedReplay,audioReplay,audioReplaySize);FreeMem(audioReplay,audioReplaySize);audioReplay=ownedReplay;
#endif
    if(!NativeAudio::prepare(audioPitch,[](uint32_t type,int16_t id,const uint8_t*& bytes,uint32_t& size){OriginalApplication::Resource r;if(!app.resource(type,id,r))return false;bytes=r.bytes;size=r.size;return true;},validMemory))return fail(NativeAudio::error());
#endif
#ifdef POP2_UI_PROBE
    g_uiText=(uint32_t)heap.newPtr(64,true);if(!g_uiText)return fail("UI / PROBE TEXT MEMORY");
    const char* uiText="Port UI probe";for(unsigned i=0;i<14;++i)((char*)g_uiText)[i]=uiText[i];
    for(unsigned i=32;i<64;++i)((uint8_t*)g_uiText)[i]=0x55;
#endif
#if defined(POP2_DISPLAY_SERVICES_PROBE) || defined(POP2_DISPLAY_FRAMES_PROBE)
    uint8_t* displayProbe=heap.newPtr(512,true);
    if(!displayProbe)return fail("DISPLAY / PROBE MEMORY");
    g_displayServiceProbe=(uint32_t)displayProbe;
#ifdef POP2_DISPLAY_FRAMES_PROBE
    if(!readFile("display-frames.bin",displayFrames,displayFramesSize,1024*1024,displayFramesSHA256))return false;
    if(displayFramesSize!=8+2*(307200+2056+768) || l(displayFrames)!=0x44465231 || l(displayFrames+4)!=2)return fail("DISPLAY / FRAME INPUT");
    auto sceneColors=heap.newHandle(2056,true);if(!sceneColors)return fail("DISPLAY / SCENE COLOURS");
    g_displaySceneColors=(uint32_t)sceneColors;g_displayLogicalScreen=(uint32_t)NativeDisplay::screen();
    copy(*sceneColors,displayFrames+8+307200,2056);
#endif
    sw(displayProbe+228,384);sw(displayProbe+230,512);
#endif
    if(!segment(1))return false;
#ifdef POP2_FILE_REPLAY
    uint8_t* replay=0;uint32_t replaySize=0;
    if(!readFile("file-replay.bin",replay,replaySize,2*1024*1024,fileReplaySHA256))return false;
    uint8_t* replayHeap=heap.newPtr(replaySize,false);
    if(!replayHeap) {FreeMem(replay,replaySize);return fail("FILE REPLAY / MEMORY");}
    copy(replayHeap,replay,replaySize);FreeMem(replay,replaySize);
    if(replaySize<8 || l(replayHeap)!=0x50465231 || !l(replayHeap+4))return fail("FILE REPLAY / HEADER");
    g_replayBegin=(uint32_t)replayHeap;g_replaySize=replaySize;g_replayCount=l(replayHeap+4);
    uint32_t at=8;
    for(uint32_t i=0;i<g_replayCount;++i) {
        if(at>replaySize || replaySize-at<400)return fail("FILE REPLAY / RECORD");
        uint8_t* record=replayHeap+at;uint32_t size=l(record+4);uint16_t flags=w(record+8);
        if(size<400 || size>replaySize-at || w(record+10)>=256)return fail("FILE REPLAY / EXTENT");
        sl(record+16+18,flags&4 ? (uint32_t)(record+144) : 0);
        if(flags&8) {
            if(l(record+16+36)>size-400)return fail("FILE REPLAY / BUFFER");
            sl(record+16+32,(uint32_t)(record+400));
        }
        at+=size;
    }
    if(at!=replaySize)return fail("FILE REPLAY / TRAILING BYTES");
#endif
#ifdef POP2_RESOURCE_PROBE
    uint8_t* probe=heap.newPtr(8192,true);if(!probe)return fail("RESOURCE PROBE / MEMORY");
    const char* paths[]={":Data:Prince.rsrc",":Data:Kid.rsrc",":Data:NIS.rsrc",":Data:Prince2.opt"};
    for(unsigned i=0;i<4;++i) {
        uint8_t* p=probe+i*256;unsigned n=0;while(paths[i][n]) {p[n+1]=paths[i][n];++n;}p[0]=n;g_resourceProbeNames[i]=(uint32_t)p;
    }
    // The measured OpenResFile caller passes this Pascal name directly from
    // CODE 2. Exercise that original address rather than only copied names.
    Resource* codeResource=get(0x434f4445,2);
    if(!codeResource || !load(*codeResource))return false;
    uint8_t* codeName=*codeResource->handle+0x4284;
    const char* optionName=paths[3];unsigned optionLength=0;
    while(optionName[optionLength])++optionLength;
    if(codeName[0]!=optionLength)return fail("RESOURCE PROBE / CODE NAME GUARD");
    for(unsigned i=0;i<optionLength;++i)if(codeName[i+1]!=optionName[i])return fail("RESOURCE PROBE / CODE NAME GUARD");
    g_resourceProbeNames[3]=(uint32_t)codeName;
    g_resourceProbeInfo=(uint32_t)(probe+1024);
    const char* specs[]={":",":a",":Data",":Data:a",":Data:Mohawk.ini",":MIDIMAP.DAT",":MOHAWK.INI",
        ":Data:MIDISnd.dat",":Data:DigiSnd.dat",":Data:NISDigi.dat",":Data:NISMidi.dat"};
    g_specProbeCount=11;
    for(unsigned i=0;i<11;++i) {
        volatile uint8_t* p=probe+2048+i*256;unsigned n=0;
        while(specs[i][n]) {p[n+1]=specs[i][n];++n;}p[0]=n;
        g_specProbePaths[i]=(uint32_t)p;g_specProbeOutputs[i]=(uint32_t)(probe+5120+i*70);g_specProbeResults[i]=32767;
    }
#endif
#ifdef POP2_FILE_PROBE
    uint8_t* pb=heap.newPtr(1024,true);if(!pb)return fail("FILE PROBE / MEMORY");
    g_fileProbePB=(uint32_t)pb;g_fileProbeBuffer=(uint32_t)(pb+256);
    const char* name=":Data:Prince2.opt";
#ifdef POP2_SETTINGS_WRITE
    g_fileProbeWriteMode=1;
#endif
#ifdef POP2_ASSET_WRITE_CONTROL
    name=":Data:MIDISnd.dat";g_fileProbeWriteMode=2;
#endif
    unsigned n=0;while(name[n]) {pb[129+n]=name[n];++n;}
    pb[128]=n;sl(pb+18,(uint32_t)(pb+128));pb[27]=g_fileProbeWriteMode ? 3 : 1;
    sl(pb+32,g_fileProbeBuffer);sl(pb+36,16);
    g_fileProbeFCB=(uint32_t)(pb+384);
    const char* midi=":Data:MIDISnd.dat";n=0;
    while(midi[n]) {pb[513+n]=midi[n];++n;}pb[512]=n;
    sl(pb+384+18,(uint32_t)(pb+512));pb[384+27]=3;
#endif
    g_coreReason="CORE / RUNNING";return true;
}
bool suspendVectors() {
    if(!vectorsInstalled)return false;
    Disable();for(unsigned i=0;i<11;++i)vectorTable[vectorIndices[i]]=savedVectors[i];
    vectorsInstalled=false;Enable();return true;
}
void resumeVectors() {
    Disable();for(unsigned i=0;i<11;++i)vectorTable[vectorIndices[i]]=portVectors[i];
    vectorsInstalled=true;Enable();
}
void execute() {
    vectorTable=(volatile uint32_t*)AmigaHardware::getVBR();
    void (*handlers[])()={pop2_exception_2,pop2_exception_3,pop2_exception_4,pop2_exception_5,
        pop2_exception_6,pop2_exception_7,pop2_exception_8,pop2_exception_9,pop2_line_a,pop2_exception_11,pop2_exception_14};
    for(unsigned i=0;i<11;++i) { savedVectors[i]=vectorTable[vectorIndices[i]];portVectors[i]=(uint32_t)handlers[i]; }
    resumeVectors();
#ifdef POP2_UNKNOWN_TRAP
    pop2_call_mac((void*)pop2_unknown_fixture,a5,stack+65536);
#elif defined(POP2_ILLEGAL)
    pop2_call_mac((void*)pop2_illegal_fixture,a5,stack+65536);
#else
    pop2_call_mac((void*)pop2_identity_fixture,a5,stack+65536);
#endif
#ifdef POP2_AUDIO_PROBE
    NativeAudio::replay(true);
#ifdef POP2_AUDIO_ORDER_PROBE
    // Allocate before resolving Jnth: newPtr may relocate unlocked resources.
    uint8_t* pcm=heap.newPtr(64,true);
    if(!pcm){fail("AUDIO / CALLBACK ORDER MEMORY");didStop=true;}
    if(pcm)for(unsigned i=0;i<64;++i)pcm[i]=0x80;
#endif
#ifdef POP2_AUDIO_BAD_RESOURCE
    g_audioLookupType=0x4d445256;
#endif
    if(!didStop)pop2_call_mac((void*)pop2_audio_lookup,a5,stack+65536);
#ifdef POP2_AUDIO_BAD_THUNK
    if(!didStop){((uint8_t*)g_audioEntry)[1]^=1;flushCode();}
#endif
#ifdef POP2_AUDIO_ORDER_PROBE
    // Authored replacement-effect fixture, paired with mac_audio.lua.
    uint8_t* replacement=audioPacket+64;
    const uint32_t selectors[]={21,22,17,17,20,20,22};g_audioCalls=7;
    for(unsigned index=0;index<7 && !didStop;++index){
        if(index==0){sw(audioPacket,5);sw(audioPacket+2,5);sw(audioPacket+4,1);}
        if(index==2){
            for(unsigned i=0;i<128;++i)audioPacket[i]=0;
            for(unsigned n=0;n<2;++n){uint8_t* packet=audioPacket+n*64;
                sl(packet,(uint32_t)pcm);sl(packet+4,64);sl(packet+8,0x2b110000);}
            sw(audioPacket+24,3);sl(audioPacket+30,(uint32_t)pop2_audio_test_callback);
            sl(audioPacket+34,(uint32_t)replacement);sw(replacement+24,5);
        }
        if(index==4 || index==5)sw(audioPacket+24,index==4?7:5);
        g_audioSelector=selectors[index];g_audioArgument=(uint32_t)(index==3?replacement:audioPacket);
        pop2_call_mac((void*)pop2_audio_invoke,a5,stack+65536);
        bool preserved=true;for(unsigned reg=0;reg<13;++reg)if(g_audioBefore[reg]!=g_audioAfter[reg])preserved=false;
        if(!didStop){if(preserved)++g_audioABI;else {fail("AUDIO / ABI REGISTERS");didStop=true;}}
        auto* row=g_audioRows[index];row[0]=g_audioResult;row[1]=NativeAudio::state().effectID;
        row[2]=w(replacement+24);row[3]=l(replacement+40);row[4]=g_audioCallbacks;row[5]=g_audioABI;
        if(!didStop)++g_audioDone;
    }
    if(!didStop){
        bool matched=g_audioDone==7 && g_audioCallbacks==1 && l(replacement+40)==1 && w(replacement+24)==7
            && g_audioRows[4][0]==0 && g_audioRows[5][0]==1;
        fail(matched?"AUDIO / CALLBACK ORDER COMPLETE":"AUDIO / CALLBACK ORDER MISMATCH");didStop=true;
    }
#else
    const uint32_t audioCount=l(audioReplay+4);g_audioCalls=audioCount;
    for(unsigned index=0;index<audioCount && !didStop;++index){
        const uint8_t* r=audioReplay+12+index*64;
        if(!NativeAudio::advanceTo(l(r))){fail(NativeAudio::error());didStop=true;break;}
        g_audioSelector=l(r+4);g_audioArgument=l(r+8);copy(audioPacket,r+16,40);
        if(g_audioSelector==17){
            uint32_t offset=l(r+56),size=l(r+60);
            if(offset>l(audioReplay+8) || size>l(audioReplay+8)-offset){fail("AUDIO / REPLAY SAMPLE");didStop=true;break;}
            sl(audioPacket,(uint32_t)(audioReplay+12+audioCount*64+offset));
            sl(audioPacket+20,(uint32_t)(audioPacket+64));
            sl(audioPacket+30,(uint32_t)pop2_audio_test_callback);sl(audioPacket+34,(uint32_t)(audioPacket+68));
        }
        if(g_audioSelector==18 && l(audioPacket+30)){sl(audioPacket+30,(uint32_t)pop2_audio_test_callback);sl(audioPacket+34,(uint32_t)(audioPacket+68));}
        if(g_audioSelector==9 || g_audioSelector==12 || g_audioSelector==17 || g_audioSelector==18 || g_audioSelector==20 || g_audioSelector==21 || g_audioSelector==27)g_audioArgument=(uint32_t)audioPacket;
#ifdef POP2_AUDIO_BAD_SELECTOR
        if(index==7)g_audioSelector=38;
#endif
#ifdef POP2_AUDIO_BAD_CALLBACK
        if(g_audioSelector==17)sl(audioPacket+30,0x00bad000);
#endif
        pop2_call_mac((void*)pop2_audio_invoke,a5,stack+65536);
        bool preserved=true;for(unsigned reg=0;reg<13;++reg)if(g_audioBefore[reg]!=g_audioAfter[reg])preserved=false;
        if(!didStop){if(preserved)++g_audioABI;else {fail("AUDIO / ABI REGISTERS");didStop=true;}}
        const auto& audioState=NativeAudio::state();
        auto* row=g_audioRows[index];row[0]=g_audioResult;row[1]=int32_t(audioState.selected);
        row[2]=audioState.selected<0?0:audioState.cursors[audioState.selected].step;
        row[3]=audioState.voices.effect.duration>=0;row[4]=g_audioCallbacks;row[5]=g_audioABI;
        if(!didStop)++g_audioDone;
    }
    if(!didStop){fail("AUDIO / FIXTURE COMPLETE");didStop=true;}
#endif
#endif
#ifdef POP2_RESOURCE_PROBE
    if(!didStop)pop2_call_mac((void*)pop2_resource_fixture,a5,stack+65536);
#endif
#ifdef POP2_FILE_REPLAY
    if(!didStop)pop2_call_mac((void*)pop2_replay_fixture,a5,stack+65536);
#endif
#ifdef POP2_FILE_PROBE
    if(!didStop)pop2_call_mac((void*)pop2_file_fixture,a5,stack+65536);
#endif
#if defined(POP2_DISPLAY_SERVICES_PROBE) || defined(POP2_DISPLAY_FRAMES_PROBE)
    if(!didStop)pop2_call_mac((void*)pop2_display_fixture,a5,stack+65536);
    if(!didStop)pop2DisplayServicesComplete();
#ifdef POP2_DISPLAY_BAD_SELECTOR
    if(!didStop)pop2_call_mac((void*)pop2_display_rejection_fixture,a5,stack+65536);
#endif
#ifdef POP2_DISPLAY_FRAMES_PROBE
    if(!didStop)pop2_call_mac((void*)pop2_display_scene_init,a5,stack+65536);
    for(unsigned scene=0;scene<2 && !didStop;++scene) {
        const uint8_t* record=displayFrames+8+scene*(307200+2056+768);
        uint8_t* destination=(uint8_t*)l((uint8_t*)g_displayServiceProbe+236);
        for(unsigned y=0;y<384;++y)copy(destination+y*528,record+(y+48)*640+65,512);
        uint8_t* clut=*(MacHeap::Handle)g_displaySceneColors;copy(clut,record+307200,2056);
        sw(clut+4,0);for(unsigned i=0;i<256;++i)sw(clut+8+i*8,i);
        pop2_call_mac((void*)pop2_display_scene_frame,a5,stack+65536);
        NativeDisplay::wait();
        uint32_t field=g_scaffoldFields;while(g_scaffoldFields-field<4) {__asm__ volatile("nop" ::: "memory");}
        if(!didStop) {
            while(g_displayParity!=0) {__asm__ volatile("nop" ::: "memory");}
            g_displayScene=scene+1;pop2DisplaySceneComplete();
            while(g_displayParity!=1) {__asm__ volatile("nop" ::: "memory");}
            pop2DisplayOddFieldComplete();
        }
    }
#endif
    if(!didStop)fail("DISPLAY / FIXTURE COMPLETE");
#else
    if(!didStop) {
        pop2_call_mac((void*)(g_coreSegments[1][0]+16),a5,stack+65536);
        if(!didStop)fail("CORE / ORIGINAL RETURN WITHOUT STOP");
    }
    #endif
#ifdef POP2_UI_PROBE
    if(didStop && g_coreStopTrap==0xaaa0 && g_coreStopSegment==17 && g_coreStopOffset==0x820 && g_coreCount==144){
        didStop=false;pop2_call_mac((void*)pop2_ui_fixture,a5,stack+65536);
        if(!didStop){NativeDisplay::wait();pop2UIFixtureComplete();fail("UI / FIXTURE COMPLETE");}
    }
#endif
#if defined(POP2_INTEGRATION_PROBE) && defined(POP2_VERIFY)
    g_coreCodeAudit=auditCode();
#endif
    suspendVectors();g_coreVectorRestored=1;
    for(unsigned i=0;i<11;++i)if(vectorTable[vectorIndices[i]]!=savedVectors[i])g_coreVectorRestored=0;
#ifdef POP2_VERIFY
    g_coreHeapOK=heap.check();
    if((g_coreStopTrap==0xa86e && g_coreStopSegment==2 && g_coreStopOffset==0x3e90)
#ifdef POP2_UI
       || (g_coreStopTrap==0xaaa0 && g_coreStopSegment==17 && g_coreStopOffset==0x820)
#endif
       ) {
#ifdef POP2_BAD_CODE_WRITE
        ((uint8_t*)g_coreSegments[2][0])[4]^=1;
#endif
        g_coreCodeAudit=auditCode();
    }
#endif
}
void release() {
#ifdef POP2_AUDIO
    NativeAudio::release();
#endif
    if(audioPitch)FreeMem(audioPitch,audioPitchSize);audioPitch=nullptr;
    NativeDisplay::reset();NativeResources::release();app.close();heap.reset();
    if(uiPlan)FreeMem(uiPlan,uiPlanSize);uiPlan=nullptr;
    if(initial)FreeMem(initial,g_coreBelow);if(stack)FreeMem(stack,65536);if(world)FreeMem(world,worldSize);
    if(displayFrames)FreeMem(displayFrames,displayFramesSize);displayFrames=nullptr;
    if(videoTransfer)FreeMem(videoTransfer,videoTransferSize);videoTransfer=nullptr;
    if(arena)FreeMem(arena,g_coreZoneBytes);if(plan)FreeMem(plan,planSize);if(file)FreeMem(file,fileSize);
    initial=stack=world=arena=plan=file=0;
}
}
#ifdef POP2_INTEGRATION_PROBE
static void integrationPress(uint32_t raw,uint32_t waitTicks){
    g_integrationPulseRaw=128;g_integrationPulseStart=NativeInput::ticks()+waitTicks+2;
    g_integrationPulseEnd=g_integrationPulseStart+4;g_integrationPulseRaw=raw;
}
extern "C" void pop2IntegrationOpeningHook(){
    NativeDisplay::wait();
#ifdef POP2_TIMING_TM_PROBE
    uint8_t* records=heap.newPtr(7*64,true);
    if(!records){fail("TIMING / TIME FIXTURE MEMORY");return;}
    g_timingTmRecords=(uint32_t)records;
    for(unsigned i=0;i<7;++i)sl(records+i*64+6,(uint32_t)pop2_timing_tm_callback);
    sw(records+6*64+60,20);
    pop2_timing_tm_fixture();
#endif
#ifdef POP2_TIMING_VBL_PROBE
    uint8_t* records=heap.newPtr(160,true);
    if(!records){fail("TIMING / FIXTURE MEMORY");return;}
    g_timingVblRecords=(uint32_t)records;
    const uint16_t counts[]={1,2,0,0xffff,0};
    for(unsigned i=0;i<5;++i){uint8_t* task=records+32*i;
        sw(task+4,1);sl(task+6,(uint32_t)pop2_timing_vbl_callback);sw(task+10,counts[i]);}
    pop2_timing_vbl_fixture();
#endif
#ifdef POP2_INTEGRATION_ESCAPE
    integrationPress(0x45,0);
#endif
}
extern "C" void pop2IntegrationRoomHook(){
    g_integrationPulseRaw=128;NativeInput::flush(0xffff,0);NativeDisplay::wait();
#ifdef POP2_GAMEPLAY_RESTART_PROBE
    ++g_gameplayRooms;
#ifdef POP2_GAMEPLAY_SAVE_PROBE
    g_integrationStage=g_gameplayRooms==1?3:5;
#else
    g_integrationStage=g_gameplayRooms==1?3:4;
#endif
    if(g_gameplayRooms==1){
#ifdef POP2_GAMEPLAY_SAVE_PROBE
        // The save fixture feeds measured keys at original ReadKeyboard steps.
#else
        integrationPress(0x13,60);
#endif
    }
    recordIntegrationStage();
#else
    g_integrationRightAt=NativeInput::ticks()+120;g_integrationStage=3;recordIntegrationStage();
#endif
}
#ifdef POP2_GAMEPLAY_SAVE_PROBE
static void gameplayEdges(uint8_t first,bool downFirst,uint8_t second=128,bool downSecond=false){
    NativeInput::inject(first,downFirst);
    if(second<128)NativeInput::inject(second,downSecond);
    uint32_t tick=NativeInput::ticks();while(NativeInput::ticks()==tick){}
}
static bool gameplayLeft=false;
static void gameplayKeyboard(uint16_t step){
    bool left=step>=14 && step<=17;
    if(left!=gameplayLeft){gameplayEdges(0x4f,left);gameplayLeft=left;}
    if(step==12 && !(g_gameplayInputActions&1)){
        gameplayEdges(0x66,true,0x21,true);g_gameplayInputActions|=1;
    }
    if(step==23 && !(g_gameplayInputActions&2)){
        gameplayEdges(0x66,true);g_gameplayInputActions|=2;
    }
    if(step==24 && !(g_gameplayInputActions&4)){
        gameplayEdges(0x18,true);gameplayEdges(0x18,false,0x66,false);g_gameplayInputActions|=4;
    }
}
extern "C" void pop2GameplayPickerShown(){integrationPress(0x44,120);}
extern "C" void pop2GameplaySaved(uint32_t result){
    g_gameplaySaveResult=result&65535;g_gameplaySavedAt=NativeInput::ticks();g_integrationPulseRaw=128;
    pop2GameplayFileCheckpoint();
}
extern "C" void pop2GameplayLoaded(uint32_t result){g_gameplayLoadResult=result&65535;pop2GameplayFileCheckpoint();}
#endif
extern "C" void pop2IntegrationDeathHook(uint8_t* originalSP){
    if(g_integrationStage==3 && w(originalSP+4)==3 && !g_integrationDeathReturn){
        g_integrationDeathKind=w(originalSP+4);g_integrationDeathReturn=l(originalSP);
        sl(originalSP,(uint32_t)pop2_integration_death_return);
    }
}
extern "C" void pop2IntegrationDeathReturned(){
    NativeDisplay::wait();g_integrationStage=4;recordIntegrationStage();
}
extern "C" void pop2IntegrationTitleHook(uint8_t* originalSP){
    uint16_t phase=w(originalSP+4);
    g_integrationTitlePhase=phase;g_integrationTitleCue=w(a5-0x1f6e);
    if(phase==2 && g_integrationStage==0){
        g_integrationPulseRaw=128;g_integrationStage=1;recordIntegrationStage();
    }
    // CODE 15+70ca/70cc changes cue 101 to 114 while selecting the logo.
    // At the next callback the original frame has returned. Its frame number
    // depends on service cost, so schedule test input from the reached cue.
    // The result checker still compares the captured reference pixels.
    if(phase>=3 && w(a5-0x1f6e)==0x72 && g_integrationStage==1){
        integrationPress(0x40,120);g_integrationStage=2;recordIntegrationStage();
    }
}
#endif
extern "C" uint32_t pop2Dispatch(uint32_t* regs,uint8_t* frame,uint8_t* usp) {
    uint32_t pc=l(frame+2),seg=0,off=0;
    if(usp<stack || usp>stack+65536-4)return stop(frame,usp,"STACK / TRAP ARGUMENT EXTENT",0,seg,off);
#ifdef POP2_UI
    bool cachedCaller=pc>=(uint32_t)pop2_cached_services && pc<(uint32_t)pop2_cached_services_end && !((pc-(uint32_t)pop2_cached_services)&3);
#else
    bool cachedCaller=false;
#endif
    // Native cached veneers cannot belong to an original CODE segment.
    // Their original return owner is still checked below.
    bool codeCaller=!cachedCaller && location(pc,seg,off);
    bool tableCaller=pc>=(uint32_t)a5+app.world().jumpOffset && pc<(uint32_t)a5+app.world().jumpOffset+app.world().jumpBytes;
    bool fixtureCaller=(pc>=(uint32_t)pop2_audio_lookup && pc<(uint32_t)pop2_audio_fixture_end) || (pc>=(uint32_t)pop2_ui_fixture && pc<(uint32_t)pop2_ui_fixture_end) || (pc>=(uint32_t)pop2_display_scene_init && pc<(uint32_t)pop2_display_scene_end) || (pc>=(uint32_t)pop2_display_fixture && pc<(uint32_t)pop2_display_fixture_end) || (pc>=(uint32_t)pop2_identity_fixture && pc<(uint32_t)pop2_identity_fixture_end) ||
        (pc>=(uint32_t)pop2_file_fixture && pc<(uint32_t)pop2_file_fixture_end) ||
        (pc>=(uint32_t)pop2_replay_fixture && pc<(uint32_t)pop2_replay_fixture_end) ||
        (pc>=(uint32_t)pop2_resource_fixture && pc<(uint32_t)pop2_resource_fixture_end);
#ifdef POP2_TIMING_TM_PROBE
    bool timingTmCaller=pc>=(uint32_t)pop2_timing_tm_fixture && pc<(uint32_t)pop2_timing_tm_end;
    if(timingTmCaller){
        if(!g_timingTmRecords || (w(frame)&0x2000))return stop(frame,usp,"TIMING / NONUSER TIME FIXTURE",0,seg,off);
        fixtureCaller=true;
        if(pc>=(uint32_t)pop2_timing_tm_callback)++g_timingTmUserCalls;
        if(w((uint8_t*)pc)==0xaffa){
            g_timingTmResult=regs[0]?regs[0]:1;
            return stop(frame,usp,regs[0]?"TIMING / TIME FIXTURE ABI FAILURE":"TIMING / TIME MANAGER FIXTURE COMPLETE",0xaffa,seg,off);
        }
    }
#endif
#ifdef POP2_TIMING_VBL_PROBE
    bool timingCaller=pc>=(uint32_t)pop2_timing_vbl_fixture && pc<(uint32_t)pop2_timing_vbl_end;
    if(timingCaller){
        if(!g_timingVblRecords || (w(frame)&0x2000))return stop(frame,usp,"TIMING / NONUSER FIXTURE",0,seg,off);
        fixtureCaller=true;
        if(pc>=(uint32_t)pop2_timing_vbl_callback)++g_timingVblUserCalls;
        if(w((uint8_t*)pc)==0xaffb){
            g_timingVblResult=regs[0]?regs[0]:1;
            return stop(frame,usp,regs[0]?"TIMING / FIXTURE ABI FAILURE":"TIMING / VBL FIXTURE COMPLETE",0xaffb,seg,off);
        }
    }
#endif
    bool audioCaller=false;
#ifdef POP2_AUDIO
    // Original CODE, port veneers and the separately allocated Jnth resource
    // have disjoint ownership. Resolve known veneers before searching resources.
    if(!codeCaller && !tableCaller && !fixtureCaller && !cachedCaller &&
       pc!=(uint32_t)pop2_old_loadseg && pc!=(uint32_t)pop2_old_exit)for(unsigned i=0;i<resourceCount;++i){auto& r=resources[i];
        if(r.type==0x4a6e7468 && r.id==11 && r.identity==0xfffffff0UL && r.handle && *r.handle && pc==(uint32_t)*r.handle){
            if(heap.handleSize(r.handle)!=4 || l(*r.handle)!=0xaffc4e75)return stop(frame,usp,"AUDIO / JNTH BYTES",0,seg,off);
            audioCaller=true;break;
        }
    }
#endif
    if(!codeCaller && !tableCaller && !fixtureCaller && !cachedCaller && !audioCaller && pc!=(uint32_t)pop2_old_loadseg && pc!=(uint32_t)pop2_old_exit)
        return stop(frame,usp,"TRAP / UNKNOWN CALLER",0,seg,off);
    uint16_t trap=w((uint8_t*)pc);
#ifdef POP2_INTEGRATION_PROBE
#ifdef POP2_GAMEPLAY_RESTART_PROBE
#ifdef POP2_GAMEPLAY_SAVE_PROBE
    if(g_integrationStage==4)return stop(frame,usp,"GAMEPLAY / SAVE LOAD COMPLETE",trap,seg,off);
#else
    if(g_integrationStage==4)return stop(frame,usp,"GAMEPLAY / RESTART COMPLETE",trap,seg,off);
#endif
#else
    if(g_integrationStage==4)return stop(frame,usp,"INTEGRATION / ROUTE COMPLETE",trap,seg,off);
#endif
#ifdef POP2_GAMEPLAY_SAVE_PROBE
    // Observe actual SaveGame/ResumeGame returns to their original dialog
    // callers. Only diagnostic return addresses change; registers/CCR survive.
    for(uint32_t at=regs[14],depth=0;depth<32 && validMemory(at,8);++depth){
        uint32_t returned=l((uint8_t*)at+4),parent=l((uint8_t*)at);
        if(!g_gameplaySaveReturn && returned==g_coreSegments[2][0]+0xc98){g_gameplaySaveReturn=returned;sl((uint8_t*)at+4,(uint32_t)pop2_gameplay_saved);}
        if(!g_gameplayLoadReturn && returned==g_coreSegments[2][0]+0xc18){g_gameplayLoadReturn=returned;sl((uint8_t*)at+4,(uint32_t)pop2_gameplay_loaded);}
        if(parent<=at)break;at=parent;
    }
#endif
    // Match the reference opening-text return, before issuing its one Escape.
    // The drawing/unlock callee returns to CODE 15+0404/0410; the parent
    // frame identifies TEXT 6978/1. Unlock can return without calling a service.
    if(!g_integrationOpeningReturn && g_coreSegments[15][0] && seg==17 &&
       (off==0x4ff4 || off==0x4dac || off==0x4dbc)){
        uint32_t at=regs[14];
        for(unsigned depth=0;depth<16 && validMemory(at,8);++depth){
            uint32_t parent=l((uint8_t*)at);
            if((l((uint8_t*)at+4)==g_coreSegments[15][0]+0x410 ||
                l((uint8_t*)at+4)==g_coreSegments[15][0]+0x404) && validMemory(parent,12) &&
               w((uint8_t*)parent+8)==0x6978 && w((uint8_t*)parent+10)==1){
                g_integrationOpeningReturn=l((uint8_t*)at+4);
                sl((uint8_t*)at+4,(uint32_t)pop2_integration_opening);break;
            }
            if(parent<=at)break;at=parent;
        }
    }
    if(seg==17 && off==0x313e &&
#ifndef POP2_GAMEPLAY_RESTART_PROBE
       g_integrationStage<3 && !g_integrationRoomReturn &&
#endif
       validMemory(regs[14],8) && l((uint8_t*)regs[14]+4)==g_coreSegments[2][0]+0x6054 && uint16_t(regs[4])==0xfffe){
        g_integrationRoomReturn=l((uint8_t*)regs[14]+4);
        sl((uint8_t*)regs[14]+4,(uint32_t)pop2_integration_room);
    }
#endif
    if((trap&0x0800) && !fixtureCaller && usp>stack+65536-10)
        return stop(frame,usp,"STACK / TOOLBOX ARGUMENT EXTENT",trap,seg,off);
    bool builtin=cachedCaller || pc==(uint32_t)pop2_old_loadseg || pc==(uint32_t)pop2_old_exit;
    if(builtin)trap=w((uint8_t*)pc+2);
#ifdef POP2_TIMING_PROBE
    g_timingPhase=trap&0xfff;
#endif
    if(codeCaller) {
#ifdef POP2_VERIFY
        uint32_t* row=g_coreTrace[(g_coreCount++)%traceCapacity];row[0]=seg;row[1]=off;row[2]=trap;
#endif
    } else if(!builtin && !fixtureCaller && !audioCaller && !(pc>=(uint32_t)a5+app.world().jumpOffset && pc<(uint32_t)a5+app.world().jumpOffset+app.world().jumpBytes && trap==0xa9f0))
        return stop(frame,usp,"TRAP / UNKNOWN CALLER",trap,seg,off);
    if(cachedCaller){
        uint32_t caller=l(usp),callerSegment=0,callerOffset=0;
        if(!location(caller,callerSegment,callerOffset) && !(caller>=(uint32_t)pop2_ui_fixture && caller<(uint32_t)pop2_ui_fixture_end))return stop(frame,usp,"TRAP / CACHED RETURN OWNER",trap,seg,off);
        sl(frame+2,caller);usp+=4;
    }else sl(frame+2,pc+(builtin?4:2));
#ifdef POP2_AUDIO
    if(audioCaller){
        uint32_t caller=l(usp),callerSeg=0,callerOff=0;
        bool fromFixture=caller>=(uint32_t)pop2_audio_lookup && caller<(uint32_t)pop2_audio_fixture_end;
        if(!location(caller,callerSeg,callerOff) && !fromFixture)return stop(frame,usp,"AUDIO / JNTH RETURN OWNER",trap,seg,off);
        if(!validMemory((uint32_t)usp,12))return stop(frame,usp,"AUDIO / ABI STACK",trap,seg,off);
#ifdef POP2_GAMEPLAY_RESTART_PROBE
        g_gameplayAudioSelector=l(usp+4);
#endif
        bool retry=false;
        if(!NativeAudio::dispatch(l(usp+4),l(usp+8),regs[0],retry))return stop(frame,usp,NativeAudio::error(),trap,seg,off);
        if(retry){sl(frame+2,pc);return (uint32_t)usp;}
#ifdef POP2_GAMEPLAY_RESTART_PROBE
        if(g_gameplayAudioSelector==10){++g_gameplaySuspendCalls;g_gameplaySuspendClock=NativeAudio::state().clock;}
        if(g_gameplayAudioSelector==11){++g_gameplayResumeCalls;g_gameplayResumeClock=NativeAudio::state().clock;}
#endif
        return (uint32_t)usp;
    }
#endif
    if(!builtin && (trap==0xa9f0 || trap==0xa9f4)) {
        uint8_t* patch=trap==0xa9f0?patchedLoad:patchedExit;
        if(patch) { flushCode();usp-=4;sl(usp,pc+2);sl(frame+2,(uint32_t)patch);return (uint32_t)usp; }
    }
#ifdef POP2_UI
    if(trap==0xaffd){
        if(!codeCaller || seg!=17 || (off!=0x245e && off!=0x6d42))return stop(frame,usp,"UI / REPLACEMENT CALLER",trap,seg,off);
        uint16_t selector=w((uint8_t*)pc+2);
        if(selector!=(off==0x245e?1:2))return stop(frame,usp,"UI / REPLACEMENT SELECTOR",trap,seg,off);
        if(!NativeUI::replacement(selector,regs,usp+4))return stop(frame,usp,NativeUI::error(),trap,seg,off);
        sl(frame+2,pc+4);return (uint32_t)usp;
    }
#endif
    uint32_t consumed=0;int16_t error=0;bool os=(trap&0x0800)==0;
#ifdef POP2_DISPLAY
    // Reuse Alone in the Dark's LINK-frame lifetime test: a returned frame may
    // remain in stack memory or be reused, so compare SP, parent and return PC.
    // Callback frames must not prematurely close the interrupted game frame.
    if(displayBatchFrame && !g_timerCallbackActive &&
       ((uint32_t)usp>displayBatchFrame || l((uint8_t*)displayBatchFrame)!=displayBatchParent ||
        l((uint8_t*)displayBatchFrame+4)!=displayBatchReturn)) {
#ifdef POP2_TIMING_PROBE
        volatile uint32_t* timing=g_timingCount<256 ? g_timingRows[g_timingCount++] : nullptr;
        if(timing){timing[0]=w(a5-0x349a);timing[1]=wallTicks();timing[2]=g_scaffoldFields;
#ifdef POP2_VERIFY
            timing[3]=g_coreCount;
#else
            timing[3]=0;
#endif
            timing[4]=g_timerCallbacks;timing[7]=displayBatchReturn;}
#endif
#ifdef POP2_GAMEPLAY_SAVE_PROBE
        bool loadedFrame=g_gameplayLoadResult==1 && displayBatchReturn==g_coreSegments[2][0]+0x616e;
#endif
        displayBatchFrame=displayBatchParent=displayBatchReturn=0;
        // As in Alone in the Dark, finish the previous publication before
        // submitting this completed frame. The new picture can then reach
        // vertical blank while the original game continues its next step.
#ifdef POP2_TIMING_PROBE
        g_timingPhase=0x1003;
#endif
        NativeDisplay::paceFrame();
#ifdef POP2_TIMING_PROBE
        g_timingPhase=0x1002;
#endif
        if(!NativeDisplay::publish(displayBatchScene))return stop(frame,usp,NativeDisplay::error(),trap,seg,off);
        displayBatchScene=false;
#ifdef POP2_GAMEPLAY_SAVE_PROBE
        if(loadedFrame){NativeDisplay::wait();++g_gameplayLoadedFrames;g_integrationStage=4;recordIntegrationStage();}
#endif
#ifdef POP2_TIMING_PROBE
        g_timingPhase=trap&0xfff;
#endif
#ifdef POP2_TIMING_PROBE
        if(timing){timing[5]=wallTicks();timing[6]=g_scaffoldFields;}
        if(g_timingCount==6)pop2TimingWindowComplete();
#endif
    }
#ifdef POP2_UI
    if(NativeInput::error())return stop(frame,usp,NativeInput::error(),trap,seg,off);
    if(NativeUI::handles(trap)) {
#ifdef POP2_GAMEPLAY_SAVE_PROBE
        bool gameplayRead=trap==0xa976 && l(frame+2)==g_coreSegments[2][0]+0x5622 && g_integrationStage>=3;
        uint16_t gameplayStep=w(a5-0x349a);
        if(gameplayRead)gameplayKeyboard(gameplayStep);
#endif
        if(!NativeUI::dispatch(trap,regs,usp,consumed))return stop(frame,usp,NativeUI::error(),trap,seg,off);
#ifdef POP2_GAMEPLAY_RESTART_PROBE
        if(trap==0xa93e && l(usp+2))g_gameplayMenu=l(usp+2);
#ifdef POP2_GAMEPLAY_SAVE_PROBE
        if(gameplayRead){
            if(g_gameplayKeyCount>=128)return stop(frame,usp,"GAMEPLAY / KEY TRACE CAPACITY",trap,seg,off);
            uint8_t* keys=(uint8_t*)l(usp);volatile uint32_t* row=g_gameplayKeys[g_gameplayKeyCount++];
            row[0]=gameplayStep;row[1]=keys[15]&8;row[2]=keys[15]&16;row[3]=keys[6]&128;
            if(gameplayStep==12 && (g_gameplayInputActions&1))gameplayEdges(0x21,false,0x66,false);
        }
#endif
#endif
        if(os){uint16_t sr=w(frame)&~15;uint16_t result=regs[0];if(!result)sr|=4;if(result&0x8000)sr|=8;sw(frame,sr);}
        return (uint32_t)(usp+consumed);
    }
#endif
#if defined(POP2_UI_PROBE) || defined(POP2_STARTUP_CHECKPOINT)
    // Preserve the bounded earlier service fixtures after adding Palette2CTab.
    if(trap==0xaaa0 && seg==17 && off==0x820)
        return stop(frame,usp,"UI / STARTUP CHECKPOINT",trap,seg,off);
#endif
    if(NativeDisplay::handles(trap)) {
        // FrameAdv can update the health display before calling DrawWorld.
        // Its leaf callees do not add LINK frames: identify FrameAdv by its
        // byte-verified caller return, before DrawWorld's GetGWorld.
        // As in the donor's frame guard, retain the owning LINK frame, parent
        // and return address until that original invocation has completed.
        if(!displayBatchFrame && !g_timerCallbackActive && g_coreSegments[2][0]) {
            uint32_t base=g_coreSegments[2][0],at=regs[14];
            for(unsigned depth=0;depth<16 && !(at&1) && at>=(uint32_t)usp && at<=(uint32_t)stack+65536-8;++depth) {
                uint32_t parent=l((uint8_t*)at),ret=l((uint8_t*)at+4);
                if(ret==base+0x616e) {
                    if(parent<=at || parent>(uint32_t)stack+65536-8 || (parent&1) ||
                       g_coreSegments[2][1]<0x68b4 || l((uint8_t*)base+0x616a)!=0x4eba05e0 ||
                       l((uint8_t*)base+0x674c)!=0x4e56fff8 ||
                       w((uint8_t*)base+0x68b2)!=0x4e75)
                        return stop(frame,usp,"FRAME / FRAMEADV OWNER",trap,seg,off);
                    if(!NativeDisplay::publish())return stop(frame,usp,NativeDisplay::error(),trap,seg,off);
                    displayBatchFrame=at;displayBatchParent=parent;displayBatchReturn=ret;displayBatchScene=false;
                    break;
                }
                if(parent<=at)break;
                at=parent;
            }
        }
        // The scene renderer itself is a leaf. Its caller owns the LINK frame
        // which also encloses the later dirty-rectangle screen-copy pass.
        if(trap==0xab1d && seg==17 && off==0x4d18 && g_coreSegments[3][0]) {
            uint32_t wrapper=regs[14],scene=g_coreSegments[3][0];
            if(!validMemory(wrapper,8))return stop(frame,usp,"FRAME / STACK EXTENT",trap,seg,off);
            if(l((uint8_t*)wrapper+4)==scene+0x333e && !displayBatchFrame) {
                uint32_t owner=l((uint8_t*)wrapper);
                if(regs[0]!=0x80005 || !validMemory(owner,8) || owner<=wrapper ||
                   l((uint8_t*)scene+0x3336)!=0x486dadc2 || l((uint8_t*)scene+0x333a)!=0x4ead11fa)
                    return stop(frame,usp,"FRAME / BEGIN CONTRACT",trap,seg,off);
                if(!NativeDisplay::publish())return stop(frame,usp,NativeDisplay::error(),trap,seg,off);
                displayBatchFrame=owner;displayBatchParent=l((uint8_t*)owner);displayBatchReturn=l((uint8_t*)owner+4);
                // A standalone DrawWorld prepares a scene before animation.
                // Finish its large buffer synchronization at this boundary;
                // FrameAdv and ordinary full-screen animation remain deferred.
                displayBatchScene=true;
            }
        }
        if(!NativeDisplay::dispatch(trap,regs,usp,consumed))return stop(frame,usp,NativeDisplay::error(),trap,seg,off);
        if(!displayBatchFrame && !NativeDisplay::publish())return stop(frame,usp,NativeDisplay::error(),trap,seg,off);
        return (uint32_t)(usp+consumed);
    }
#endif
    if(NativeFiles::handles(trap,(uint16_t)regs[0])) {
        auto valid=[](uint32_t at,uint32_t n) {
            return (at>=(uint32_t)stack && at-(uint32_t)stack<=65536 && n<=65536-(at-(uint32_t)stack)) ||
                (at>=(uint32_t)world && at-(uint32_t)world<=worldSize && n<=worldSize-(at-(uint32_t)world)) ||
                (at>=(uint32_t)arena && at-(uint32_t)arena<=g_coreZoneBytes && n<=g_coreZoneBytes-(at-(uint32_t)arena));
        };
        if(!valid(regs[8],128))return stop(frame,usp,"FILES / PARAMETER BLOCK EXTENT",trap,seg,off);
        uint8_t* pb=(uint8_t*)regs[8];uint8_t* buffer=0;uint8_t* nameOutput=0;char path[256];const char* name=0;
        if(trap==0xa002 || trap==0xa003) {
            if(l(pb+36) && !valid(l(pb+32),l(pb+36)))return stop(frame,usp,"FILES / BUFFER EXTENT",trap,seg,off);
            buffer=(uint8_t*)l(pb+32);
        }
        if(trap==0xa000 || trap==0xa200 || trap==0xa00a || trap==0xa20a || trap==0xa008 || trap==0xa207 || trap==0xa00c || trap==0xa00d || trap==0xa014 || trap==0xa214 || trap==0xa015 || trap==0xa013 || trap==0xa060 || trap==0xa260) {
            uint32_t address=l(pb+18);
            bool query=trap==0xa014 || trap==0xa214 || (trap==0xa207 && (int16_t)w(pb+28)>=0) || (trap==0xa00c && (int16_t)w(pb+28)>0) ||
                ((trap==0xa060 || trap==0xa260) && ((uint16_t)regs[0]==8 || ((uint16_t)regs[0]==9 && (int16_t)w(pb+28)<0)));
            if(address && query) {
                if(!valid(address,256))return stop(frame,usp,"FILES / OUTPUT NAME EXTENT",trap,seg,off);
                nameOutput=(uint8_t*)address;
            } else if(address) {
                if(!valid(address,1) || !valid(address+1,*(uint8_t*)address))return stop(frame,usp,"FILES / NAME EXTENT",trap,seg,off);
                uint8_t* p=(uint8_t*)address;for(unsigned i=0;i<p[0];++i)path[i]=p[i+1];path[p[0]]=0;name=path;
                if(trap==0xa00c || trap==0xa207 || ((trap==0xa060 || trap==0xa260) && (uint16_t)regs[0]==9)) {
                    if(!valid(address,256))return stop(frame,usp,"FILES / OUTPUT NAME EXTENT",trap,seg,off);
                    nameOutput=(uint8_t*)address;
                }
            }
        }
        int32_t fileError=NativeFiles::dispatch(trap,(uint16_t)regs[0],pb,buffer,name,nameOutput);
        if(fileError==MacFiles::unsupported)return stop(frame,usp,"FILES / UNSUPPORTED OPERATION",trap,seg,off);
        sw(pb+16,fileError);regs[0]=fileError;
        uint16_t sr=w(frame)&~15;if(!(fileError&0xffff))sr|=4;if(fileError&0x8000)sr|=8;sw(frame,sr);
        return (uint32_t)usp;
    }
    switch(trap) {
    case 0xa84d:case 0xa868:
        if(!fixedService(trap,usp))goto failed;
        consumed=8;break;
    case 0xa800: {
        if(regs[0]==0x000c0008){sl(usp,0x03208000);regs[0]=0x0008000c;}
        else if(regs[0]==0x022c0018){
            if(!validMemory(l(usp),4)){fail("SOUND / VOLUME OUTPUT");goto failed;}
            sl((uint8_t*)l(usp),soundVolume);sw(usp+4,0);consumed=4;regs[0]=0;
        } else if(regs[0]==0x02300018){
            soundVolume=l(usp);
            if((soundVolume&65535)>256 || (soundVolume>>16)>256){fail("SOUND / VOLUME RANGE");goto failed;}
#ifdef POP2_AUDIO
            PaulaAudio::masterVolume(soundVolume);
#endif
            sw(usp+4,0);consumed=4;regs[0]=0;
        } else {fail("SOUND / DISPATCH SELECTOR");goto failed;}
        break;
    }
    case 0xa51e:regs[8]=(uint32_t)heap.newPtr(regs[0],true);error=heap.error();regs[0]=(int32_t)error;break;
    case 0xa033:case 0xa034: {
        if(!validMemory(regs[8],14)){fail("VBL / TASK EXTENT");goto failed;}
        uint8_t* task=(uint8_t*)regs[8];int index=vblIndex(task);
        if(trap==0xa033){
            if(index>=0 || vblCount==8 || w(task+4)!=1 || w(task+12)){fail("VBL / INSTALL CONTRACT");goto failed;}
            if(!vblCount)vblPass.reset(wallTicks());
            else sl(vblTasks[vblCount-1],(uint32_t)task);
            sl(task,0);vblTasks[vblCount++]=task;
        }else {
            if(w(task+4)!=1){regs[0]=(uint32_t)-2;break;}
            if(index<0){regs[0]=(uint32_t)-1;break;}
            removeVBL(index);
        }
        regs[0]=0;break;
    }
    case 0xa9b4: // SystemTask: callbacks are delivered by the common service return.
        regs[0]=(uint32_t)a5;break;
    case 0xa05c:
        if((regs[0]!=2 && regs[0]!=3) || !validMemory(regs[8],regs[9])){fail("MEMORY DISPATCH / LOCK RANGE");goto failed;}
        regs[0]=0;break; // LockMemory / UnlockMemory: native allocations are resident and physically stable.
    case 0xa05d:
        if((regs[0]&255)>1){fail("MMU / ADDRESS MODE");goto failed;}
        {uint8_t requested=regs[0];regs[0]=addressMode;addressMode=requested;low[0xcb2]=requested;}break; // Native pointers remain full addresses.
    case 0xa85d: { // BitTst numbers bits from the most significant bit.
        uint32_t bit=l(usp),address=l(usp+4)+(int32_t(bit)>>3);
        if(!validMemory(address,1)){fail("BITTEST / BYTE EXTENT");goto failed;}
        usp[8]=(*(uint8_t*)address&(0x80>>(bit&7)))?1:0;
        consumed=8;regs[0]=uint16_t(~bit);regs[1]=bit>>3;break;
    }
    case 0xa9c6: { // Secs2Date uses D0 seconds since 1904 and A0 DateTimeRec.
        if(!validMemory(regs[8],14)){fail("DATE / RECORD EXTENT");goto failed;}
        uint8_t* out=(uint8_t*)regs[8];uint32_t days=regs[0]/86400,seconds=regs[0]%86400;
        uint16_t weekday=(days+5)%7+1,year=1904,month=1;
        auto leap=[](uint16_t y){return !(y%4) && ((y%100) || !(y%400));};
        while(days>=uint32_t(leap(year)?366:365)){days-=leap(year)?366:365;++year;}
        const uint8_t lengths[]={31,28,31,30,31,30,31,31,30,31,30,31};
        while(days>=uint32_t(lengths[month-1]+(month==2 && leap(year)))){days-=lengths[month-1]+(month==2 && leap(year));++month;}
        sw(out,year);sw(out+2,month);sw(out+4,days+1);sw(out+6,seconds/3600);
        sw(out+8,(seconds/60)%60);sw(out+10,seconds%60);sw(out+12,weekday);break;
    }
    case 0xa058:case 0xa059:case 0xa05a:
        if(!timerService(trap,regs))goto failed;break;
    case 0xaa52: {
        if((uint16_t)regs[0]!=1 || !validMemory((uint32_t)usp,16)) {fail("FILES / FSSPEC SELECTOR OR ARGUMENTS");goto failed;}
        char path[256];if(!pascalPath(l(usp+4),path) || !validMemory(l(usp),70)) {fail("FILES / FSSPEC EXTENT");goto failed;}
        int32_t result=NativeFiles::makeSpec((int16_t)w(usp+12),l(usp+8),path,(uint8_t*)l(usp));
        if(result==MacFiles::unsupported) {fail("FILES / FSSPEC UNSUPPORTED");goto failed;}
        sw(usp+14,result);regs[0]=result;consumed=14;break;
    }
    case 0xa997: {
        char path[256];if(!pascalPath(l(usp),path)) {fail("RESOURCE / FILE NAME EXTENT");goto failed;}
        int16_t ref=-1;resourceError=NativeResources::open(0,0,path,0,ref);
        if(resourceError==MacFiles::unsupported) {fail("RESOURCE / OPEN UNSUPPORTED");goto failed;}
        sw(usp+4,ref);sw(low+0xa60,resourceError);sw(low+0xa5a,NativeResources::current());consumed=4;break;
    }
    case 0xa994:sw(usp,NativeResources::current());break;
    case 0xa9af:sw(usp,resourceError);break;
    case 0xa998:case 0xa99a: {
        int16_t ref=(int16_t)w(usp);
        resourceError=trap==0xa998 ? NativeResources::use(ref) : NativeResources::close(ref);
        if(resourceError==MacFiles::unsupported) {fail("RESOURCE / FILE OPERATION UNSUPPORTED");goto failed;}
        if(!resourceError && trap==0xa99a)for(unsigned i=0;i<resourceCount;++i) {
            Resource& r=resources[i];if(r.handle && !r.detached && r.fileRef==ref) {
                if(heap.disposeHandle(r.handle)) {fail("RESOURCE / CLOSE HANDLE");goto failed;}r.handle=0;
            }
        }
        sw(low+0xa60,resourceError);sw(low+0xa5a,NativeResources::current());if(trap==0xa99a)regs[0]=(uint16_t)resourceError;consumed=2;break;
    }
    case 0xa9a4:case 0xa9a8: {
        if(trap==0xa9a8 && !validMemory((uint32_t)usp,16)) {fail("RESOURCE / INFO ARGUMENTS");goto failed;}
        auto h=(MacHeap::Handle)l(usp+(trap==0xa9a8 ? 12 : 0));Resource* r=byHandle(h);
        NativeResources::Item item={};const uint8_t* name=0;uint8_t length=0;
        resourceError=!r || r->detached ? -192 : NativeResources::info(r->identity,item,name,length);
        if(!resourceError) {
            if(trap==0xa9a4)sw(usp+4,item.file);
            else {
                if(!validMemory(l(usp),uint32_t(length)+1) || !validMemory(l(usp+4),4) || !validMemory(l(usp+8),2)) {fail("RESOURCE / INFO OUTPUT EXTENT");goto failed;}
                uint8_t* output=(uint8_t*)l(usp);output[0]=length;copy(output+1,name,length);
                sl((uint8_t*)l(usp+4),item.type);sw((uint8_t*)l(usp+8),item.id);
            }
        } else if(trap==0xa9a4)sw(usp+4,0xffff);
        sw(low+0xa60,resourceError);if(trap==0xa9a8)regs[0]=(uint16_t)resourceError;consumed=trap==0xa9a8 ? 16 : 4;break;
    }
    case 0xa9a0: {
        Resource* r=get(l(usp+2),(int16_t)w(usp));
        if(!r && !resourceError)goto failed;
        sl(usp+6,r ? (uint32_t)r->handle : 0);regs[0]=0;
        if(!r) {consumed=6;break;}
#ifdef POP2_VERIFY
        if(g_coreResourceCount<32){ uint32_t* row=g_coreResources[g_coreResourceCount];row[0]=r->type;row[1]=(uint16_t)r->id;row[2]=(uint32_t)r->handle; }
        ++g_coreResourceCount;
#endif
        consumed=6;break;
    }
    case 0xa9a5: case 0xa9a6: case 0xa9a2: case 0xa992: case 0xa9a3: {
        auto h=(MacHeap::Handle)l(usp);Resource* r=byHandle(h);
        if(!r || !heap.isHandle(h)) { fail("RESOURCE / UNKNOWN HANDLE");goto failed; }
        if(trap==0xa9a5)sl(usp+4,r->source.size);
        if(trap==0xa9a6)sw(usp+4,r->source.attributes);
        if(trap==0xa9a2 && !load(*r))goto failed;
        if(trap==0xa992) { r->detached=true;heap.setState(h,heap.state(h)&~0x20);regs[0]=0; }
        if(trap==0xa9a3) {
#ifdef POP2_VERIFY
            if(seg==1 && off==0x120)copy(initial,world,g_coreBelow);
#endif
            if(r->detached) { fail("RESOURCE / RELEASE DETACHED");goto failed; }
            error=heap.disposeHandle(h);r->handle=0;r->detached=true;
        }
        consumed=4;break;
    }
    case 0xa02e: {
        uint32_t n=regs[0],from=regs[8],to=regs[9];
        if(!validMemory(from,n)||!validMemory(to,n)){fail("MEMORY / BLOCKMOVE EXTENT");goto failed;}
        if(to>from && to-from<n)for(uint32_t i=n;i;--i)((uint8_t*)to)[i-1]=((const uint8_t*)from)[i-1];
        else copy((uint8_t*)to,(const uint8_t*)from,n);
        regs[0]=0;break;
    }
    case 0xa31e:regs[8]=(uint32_t)heap.newPtr(regs[0],true);error=heap.error();regs[0]=(int32_t)error;break;
    case 0xa021:
        if(!validMemory(regs[8],1)){fail("HEAP / POINTER SIZE OWNER");goto failed;}
        regs[0]=heap.ptrSize((uint8_t*)regs[8]);
        if(heap.error()){
            auto handle=heap.recoverHandle((uint8_t*)regs[8]);
            if(!handle){fail("HEAP / POINTER SIZE BLOCK");goto failed;}
            regs[0]=heap.handleSize(handle);
        }
        break;
    case 0xa01f:error=heap.disposePtr((uint8_t*)regs[8]);regs[0]=(int32_t)error;break;
    case 0xa023: {
        auto h=(MacHeap::Handle)regs[8];error=heap.disposeHandle(h);regs[0]=(int32_t)error;
        if(!error){Resource* r=byHandle(h);if(r)r->handle=nullptr;}break;
    }
    case 0xa049:case 0xa04a: {
        auto h=(MacHeap::Handle)regs[8];
        if(trap==0xa049 && heap.isFreeHandleSlot(h)) {
            // MEASURED original demo cleanup: HPurge on a disposed master
            // returns memWZErr in D0/MemError, without modifying the free slot.
            regs[0]=int32_t(MacHeap::memWZErr);sw(low+0x220,MacHeap::memWZErr);
            sw(frame,(w(frame)&~15)|8);return (uint32_t)usp;
        }
        if(!heap.isHandle(h)){fail("HEAP / PURGE HANDLE OWNER");goto failed;}
        error=heap.setState(h,trap==0xa049?heap.state(h)|0x40:heap.state(h)&~0x40);regs[0]=(int32_t)error;break;
    }
    case 0xa128:regs[8]=(uint32_t)heap.recoverHandle((uint8_t*)regs[8]);error=heap.error();regs[0]=(int32_t)error;break;
    case 0xa11a:regs[8]=(uint32_t)arena;regs[0]=0;break;
    case 0xa01b:if(regs[8]!=(uint32_t)arena){fail("HEAP / UNKNOWN ZONE");goto failed;}regs[0]=0;break;
    case 0xa055:regs[0]&=0x00ffffff;break;
    case 0xa01c:regs[0]=heap.freeBytes();break;
    case 0xa11d:regs[0]=heap.compact();regs[8]=0;break;
    case 0xa162:regs[0]=heap.freeBytes();regs[8]=heap.largestBlock();break;
    case 0xa036:error=heap.moreMasters();regs[0]=(int32_t)error;break;
    case 0xa069:case 0xa06a: {
        auto h=(MacHeap::Handle)regs[8];if(!heap.isHandle(h)){fail("HEAP / STATE HANDLE OWNER");goto failed;}
        if(trap==0xa069){regs[0]=heap.state(h);error=heap.error();}
        else {error=heap.setState(h,uint8_t(regs[0]));regs[0]=(int32_t)error;}
        break;
    }
    case 0xa02a:case 0xa024: {
        auto h=(MacHeap::Handle)regs[8];if(!heap.isHandle(h)){fail("HEAP / UNKNOWN HANDLE");goto failed;}
        error=trap==0xa02a?heap.setState(h,heap.state(h)&~0x80):heap.setHandleSize(h,regs[0]);
        regs[0]=(int32_t)error;break;
    }
#ifdef POP2_INPUT
    case 0xa975: {
        uint32_t ticks=callbackTicks();sl(usp,ticks);regs[0]=0;
#if defined(POP2_TIMING_PROBE) && defined(POP2_INTEGRATION_PROBE)
        if(g_integrationStage==3 && seg==17 && off==0x313e && g_timingSetCount<512){
            if(!validMemory(regs[14],14))return stop(frame,usp,"TIMING / SETTIMER FRAME",trap,seg,off);
            volatile uint32_t* row=g_timingSetRows[g_timingSetCount++];
            row[0]=w(a5-0x349a);row[1]=ticks;row[2]=w((uint8_t*)regs[14]+8);
            row[3]=w((uint8_t*)regs[14]+12);row[4]=g_scaffoldFields;
        }
#endif
        break;
    }
#endif
    case 0xa040: // ResrvMem: reserve contiguous space in the current zone.
        error=heap.largestBlock()>=regs[0] ? 0 : MacHeap::memFullErr;regs[0]=(int32_t)error;break;
    case 0xa025: {
        auto h=(MacHeap::Handle)regs[8];if(!heap.isHandle(h)) { fail("HEAP / UNKNOWN HANDLE");goto failed; }
        regs[0]=heap.handleSize(h);error=heap.error();break;
    }
    case 0xa029: case 0xa064: {
        auto h=(MacHeap::Handle)regs[8];if(!heap.isHandle(h)) { fail("HEAP / UNKNOWN HANDLE");goto failed; }
        error=trap==0xa029?heap.setState(h,heap.state(h)|0x80):heap.moveHigh(h);regs[0]=(int32_t)error;break;
    }
    case 0xa122:regs[8]=(uint32_t)heap.newHandle(regs[0],true);error=heap.error();regs[0]=(int32_t)error;break;
    case 0xa11e: regs[8]=(uint32_t)heap.newPtr(regs[0],false);error=heap.error();regs[0]=(int32_t)error;break;
#ifdef POP2_UI
    case 0xa746:case 0xa346:{
        uint16_t target=trap==0xa746 ? 0xa800|(regs[0]&0x3ff) : 0xa000|(regs[0]&255);
        if(target==0xa0ad)target=0xa1ad;
        uint8_t* found=nullptr;
        for(uint8_t* p=(uint8_t*)pop2_cached_services;p<(uint8_t*)pop2_cached_services_end;p+=4)if(w(p+2)==target)found=p;
        if(!found)return stop(frame,usp,"TRAP TABLE / UNMEASURED QUERY",trap,seg,off);
        regs[8]=(uint32_t)found;break;
    }
#endif
    case 0xa146: case 0xa047: {
        uint16_t target=regs[0];uint8_t** patch=target==0xa9f0?&patchedLoad:target==0xa9f4?&patchedExit:0;
        if(!patch) { fail("TRAP TABLE / UNKNOWN ENTRY");goto failed; }
        if(trap==0xa047) {
            bool exitEntry=seg==19 && off==0x033c && target==0xa9f4 && regs[8]==(uint32_t)a5+0x13fa;
            if(!heap.owns((void*)regs[8]) && !exitEntry) { fail("TRAP TABLE / UNKNOWN PATCH TARGET");goto failed; }
            *patch=(uint8_t*)regs[8];
        }
        else regs[8]=(uint32_t)(*patch?*patch:target==0xa9f0?(uint8_t*)pop2_old_loadseg:(uint8_t*)pop2_old_exit);
        regs[0]=0;break;
    }
    case 0xa9f0: {
        if(!builtin) { fail("LOADSEG / ORIGINAL HOOK MISSING");goto failed; }
        uint32_t resume=l(usp)-6;int16_t id=w(usp+4);
        uint32_t begin=(uint32_t)a5+app.world().jumpOffset;
        if(resume<begin+2 || resume>=begin+app.world().jumpBytes || ((resume-begin-2)&7)) {
            fail("LOADSEG / UNKNOWN RETURN TARGET");goto failed;
        }
        if(!segment(id))goto failed;
        usp+=6;sl(frame+2,resume);return (uint32_t)usp;
    }
    case 0xa1ad: {
        uint32_t answer;if(!MacIdentity::gestalt(regs[0],answer,error)) { fail("GESTALT / UNKNOWN SELECTOR");goto failed; }
        regs[0]=(int32_t)error;regs[8]=answer;error=0;break;
    }
    case 0xa090:
        if(!((regs[8]>=(uint32_t)stack && regs[8]-(uint32_t)stack<=65536-16) ||
             (regs[8]>=(uint32_t)world && regs[8]-(uint32_t)world<=worldSize-16) ||
             (regs[8]>=(uint32_t)arena && regs[8]-(uint32_t)arena<=g_coreZoneBytes-16) ||
             regs[8]==(uint32_t)g_coreSysEnv) || !MacIdentity::sysEnvirons(regs[0],(uint8_t*)regs[8])) {
            fail("SYSENV / UNSUPPORTED RECORD");goto failed;
        }
        regs[0]=0;break;
    case 0xa8fe:return stop(frame,usp,"UNIMPLEMENTED / InitFonts",trap,seg,off);
    case 0xa86e:return stop(frame,usp,"UNIMPLEMENTED / InitGraf",trap,seg,off);
    default:return stop(frame,usp,"UNIMPLEMENTED / TRAP",trap,seg,off);
    }
    if(os && trap!=0xa058 && trap!=0xa059 && trap!=0xa05a)sw(low+0x220,heap.error());
    if(error) { fail("CORE / MEMORY SERVICE ERROR");goto failed; }
    if(os) {
        // Macintosh OS traps return TST.W D0 flags; Toolbox preserves CCR.
        uint16_t sr=w(frame)&~15;uint16_t result=regs[0];
        if(!result)sr|=4;if(result&0x8000)sr|=8;sw(frame,sr);
    }
    return (uint32_t)(usp+consumed);
failed:return stop(frame,usp,g_coreReason,trap,seg,off);
}
// The assembly trampoline parks registers on the Mac stack and calls here in
// user mode on a separate service stack. Rebuild only the original return image;
// consumed Pascal arguments may make source and destination overlap.
// Like Alone in the Dark's specialized trap entries, this selects existing
// services without copying a complete register image through the general router.
// It is limited to an already active user-mode callback. Any pending audio work
// takes the ordinary route; timer/VBL callbacks cannot nest in either route.
static uint32_t callbackService(uint32_t* parked) {
#if defined(POP2_AUDIO) && defined(POP2_INPUT)
    if(!g_timerCallbackActive || NativeAudio::servicePending())return 0;
    uint32_t pc=g_coreServicePC;
    if(!validMemory(pc,2))return 0;
    uint16_t trap=w((uint8_t*)pc);
    if(trap!=0xa058 && trap!=0xa059 && trap!=0xa05a && trap!=0xa868 && trap!=0xa84d && trap!=0xa975)return 0;
    uint32_t seg=0,off=0;if(!location(pc,seg,off))return 0;
    uint8_t* usp=(uint8_t*)parked+66;
    if(usp<stack || usp>stack+65536-4)return 0;

#ifdef POP2_VERIFY
    uint32_t* row=g_coreTrace[(g_coreCount++)%traceCapacity];row[0]=seg;row[1]=off;row[2]=trap;
#endif
    uint8_t frame[8]={};sw(frame,g_coreServiceSR);sl(frame+2,pc+2);
    uint32_t consumed=0;bool ok=true;
    g_coreServiceActive=1;
#ifdef POP2_TIMING_PROBE
    g_timingPhase=trap&0xfff;
#endif
    switch(trap) {
    case 0xa058:case 0xa059:case 0xa05a:ok=timerService(trap,parked);break;
    case 0xa868:case 0xa84d:ok=fixedService(trap,usp);consumed=ok?8:0;break;
    case 0xa975:sl(usp,callbackTicks());parked[0]=0;break;
    }
    g_coreServiceActive=0;
    if(!(trap&0x0800)) {
        // The selected OS services here are Time Manager calls, not heap calls.
        // Preserve MemError across callbacks, including a preceding HPurge error.
        uint16_t sr=w(frame)&~15,result=parked[0];
        if(!result)sr|=4;if(result&0x8000)sr|=8;sw(frame,sr);
    }
    if(!ok)stop(frame,usp,g_coreReason,trap,seg,off);
    uint8_t* image=(uint8_t*)parked+consumed;
    if(consumed)__builtin_memmove(image,parked,60);
    sw(image+60,w(frame));sl(image+62,l(frame+2));
#ifdef POP2_TIMING_PROBE
    g_timingPhase=g_timerCallbackActive?0x1001:0x1000;
#endif
    return (uint32_t)image;
#else
    return 0;
#endif
}
static int selectVBL() {
#ifdef POP2_INPUT
    if(g_clockFault){fail("TIME / NONMONOTONIC FIELD CLOCK");return -1;}
    // Installation establishes the epoch, and removing the last task resets it.
    // An empty queue needs no beam reads at every service boundary.
    if(!vblCount)return 0;
    uint32_t ticks=wallTicks();
    if(g_clockFault){fail("TIME / NONMONOTONIC FIELD CLOCK");return -1;}
    vblPass.observe(ticks);
    for(;;) {
        if(!vblPass.active) {
            if(!vblPass.begin(vblCount))return 0;
            for(unsigned i=0;i<vblPass.limit;++i) {
                uint8_t* task=vblTasks[i];
                if(!validMemory((uint32_t)task,14)){fail("VBL / QUEUED TASK EXTENT");return -1;}
                // MEASURED System 7.5.5: every count decrements modulo 16 bits;
                // only the transition to zero calls the task.
                sw(task+10,uint16_t(w(task+10)-1));
            }
        }
        while(vblPass.index<vblPass.limit) {
            uint8_t* task=vblTasks[vblPass.index++];
            if(w(task+10))continue;
            uint32_t callback=l(task+6),cs=0,co=0;
            bool dynamic=validMemory(callback,12) && w((uint8_t*)callback)==0x6104 && l((uint8_t*)callback+6)==0x225f2251 && w((uint8_t*)callback+10)==0x4ed1;
            uint32_t entry=dynamic?l((uint8_t*)callback+2):callback;
            bool jump=entry>=(uint32_t)a5+app.world().jumpOffset+2 && entry<(uint32_t)a5+app.world().jumpOffset+app.world().jumpBytes && ((entry-(uint32_t)a5-app.world().jumpOffset)&7)==2;
            bool fixture=false;
#ifdef POP2_TIMING_VBL_PROBE
            fixture=g_timingVblRecords && entry==(uint32_t)pop2_timing_vbl_callback;
#endif
            if(!location(entry,cs,co) && !jump && !fixture){fail("VBL / CALLBACK OWNER");return -1;}
            flushCode();g_timerCallbackCode=callback;g_timerCallbackTask=(uint32_t)task;
            callbackKind=2;g_timerCallbackActive=1;
#ifdef POP2_VERIFY
            ++g_timerCallbacks;
#endif
            return 1;
        }
        vblPass.active=false;
    }
#else
    return 0;
#endif
}
extern "C" int32_t pop2TimerCallbackComplete() {
#ifdef POP2_TIMING_PROBE
    g_timingPhase=0x1004;
#endif
    if(callbackKind==2) {
        int index=vblIndex((uint8_t*)g_timerCallbackTask);
        if(index>=0 && !w(vblTasks[index]+10))removeVBL(index);
    }
    g_timerCallbackActive=0;callbackKind=0;
    int result=selectVBL();
    if(result<0)didStop=true;
#ifdef POP2_TIMING_PROBE
    g_timingPhase=g_timerCallbackActive?0x1001:0x1000;
#endif
    return result;
}
extern "C" uint32_t pop2UserDispatch(uint32_t* parked) {
    if(uint32_t fast=callbackService(parked))return fast;
    uint8_t frame[8]={};sw(frame,g_coreServiceSR);sl(frame+2,g_coreServicePC);
    // As in Alone in the Dark and callbackService, service the parked registers
    // directly. Move them only when argument cleanup changes the return stack.
    uint32_t* regs=parked;
    uint8_t* usp=(uint8_t*)parked+66;
    g_coreServiceActive=1;
    uint8_t* resumed=(uint8_t*)pop2Dispatch(regs,frame,usp);
    g_coreServiceActive=0;
#ifdef POP2_AUDIO
    if(!didStop && NativeAudio::servicePending()){
        if(!NativeAudio::sync())resumed=(uint8_t*)stop(frame,resumed,NativeAudio::error(),0,0,0);
        uint32_t callback=0,context=0,cs=0,co=0;
        if(!didStop && NativeAudio::callback(callback,context)){
            bool fixtureCallback=false;
#ifdef POP2_AUDIO_PROBE
            fixtureCallback=callback==(uint32_t)pop2_audio_test_callback;
#endif
            bool jump=callback>=(uint32_t)a5+app.world().jumpOffset+2 && callback<(uint32_t)a5+app.world().jumpOffset+app.world().jumpBytes && ((callback-(uint32_t)a5-app.world().jumpOffset)&7)==2;
            if((!location(callback,cs,co) && !jump && !fixtureCallback) || !validMemory(context,1) || resumed<stack+128)
                resumed=(uint8_t*)stop(frame,resumed,"AUDIO / CALLBACK OWNER",0,0,0);
            else{
                g_audioCallbackCode=callback;g_audioCallbackContext=context;
                resumed-=4;sl(resumed,l(frame+2));sl(frame+2,(uint32_t)pop2_audio_callback);
            }
        }
    }
#endif
    if(!didStop && timerCount && !g_timerCallbackActive && l(frame+2)!=(uint32_t)pop2_audio_callback){
        TimerStamp now=timerNow();
        // Installation/removal maintain the occupied extent, including holes.
        for(unsigned i=0;i<timerCount;++i){
            auto& t=timers[i];
            if(!t.task || !t.active || !timerDue(now,t.due))continue;
            t.active=false;sw(t.task+4,w(t.task+4)&0x7fff);sl(t.task+10,0);
            uint32_t callback=l(t.task+6),cs=0,co=0;
            if(!callback)continue;
            bool jump=callback>=(uint32_t)a5+app.world().jumpOffset+2 && callback<(uint32_t)a5+app.world().jumpOffset+app.world().jumpBytes && ((callback-(uint32_t)a5-app.world().jumpOffset)&7)==2;
            bool fixture=false;
#ifdef POP2_TIMING_TM_PROBE
            fixture=g_timingTmRecords && callback==(uint32_t)pop2_timing_tm_callback;
#endif
            if((!location(callback,cs,co) && !jump && !fixture) || resumed<stack+128)
                resumed=(uint8_t*)stop(frame,resumed,"TIME / CALLBACK OWNER",0,0,0);
            else {
                g_timerCallbackCode=callback;g_timerCallbackTask=(uint32_t)t.task;
                callbackKind=1;g_timerCallbackActive=1;
#ifdef POP2_VERIFY
                ++g_timerCallbacks;
#endif
                resumed-=4;sl(resumed,l(frame+2));sl(frame+2,(uint32_t)pop2_timer_callback);
            }
            break;
        }
    }
#ifdef POP2_INPUT
    if(!didStop && !g_timerCallbackActive && l(frame+2)!=(uint32_t)pop2_audio_callback){
        if(resumed<stack+128)resumed=(uint8_t*)stop(frame,resumed,"VBL / CALLBACK STACK",0,0,0);
        else {
            int selected=selectVBL();
            if(selected<0)resumed=(uint8_t*)stop(frame,resumed,g_coreReason,0,0,0);
            else if(selected){resumed-=4;sl(resumed,l(frame+2));sl(frame+2,(uint32_t)pop2_timer_callback);}
        }
    }
#endif
    uint8_t* image=resumed-66;
    if(image!=(uint8_t*)parked)__builtin_memmove(image,parked,60);
    sw(image+60,w(frame));sl(image+62,l(frame+2));
#ifdef POP2_TIMING_PROBE
    g_timingPhase=g_timerCallbackActive?0x1001:0x1000;
#endif
    return (uint32_t)image;
}
extern "C" uint32_t pop2Exception(uint8_t* frame) {
    g_coreServiceActive=0;
    g_coreFaultPC=l(frame+2);uint32_t seg=0,off=0;location(g_coreFaultPC,seg,off);
    const char* why="EXCEPTION / UNKNOWN";
    switch(g_coreException) {
    case 2:why="EXCEPTION / BUS ERROR";break;case 3:why="EXCEPTION / ADDRESS ERROR";break;
    case 4:why="EXCEPTION / ILLEGAL INSTRUCTION";break;case 5:why="EXCEPTION / DIVIDE BY ZERO";break;
    case 6:why="EXCEPTION / CHK";break;case 7:why="EXCEPTION / TRAPV";break;
    case 8:why="EXCEPTION / PRIVILEGE VIOLATION";break;case 9:why="EXCEPTION / TRACE";break;
    case 11:why="EXCEPTION / LINE F";break;case 14:why="EXCEPTION / FORMAT ERROR";break;
    }
    uint16_t sr=w(frame);uint32_t format=w(frame+6)>>12,bytes=8;
    if(format==2)bytes=12;else if(format==9)bytes=20;else if(format==10)bytes=32;else if(format==11)bytes=92;
    // A fault in a service unwinds to the original Exec supervisor stack.
    // No original instruction is resumed after any exception.
    if(g_coreActiveFrame) { frame=(uint8_t*)g_coreActiveFrame;sr=w(frame);g_coreActiveFrame=0; }
    else frame+=bytes-8;
    sw(frame,sr&0x1fff);sw(frame+6,0);
    stop(frame,0,why,0,seg,off);return (uint32_t)frame;
}
#endif
