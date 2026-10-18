#include "../../mac/FreedHandleCall.h"
#include "../../mac/LineAVector.h"
extern "C" volatile unsigned long pop2WHDLoadBinding[4];
#ifdef POP2_CORE
#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/memory.h>
#include "CoreRuntime.h"
#include "SaveRectPatch.h"
#include "BankSkipPatch.h"
#include "mac/VideoTicks.h"
#include "mac/BigEndianMemory.h"
#include "mac/BlockMoveBytes.h"
#ifdef POP2_QUICK_TRAP_PROBE
#include "mac/BlockMoveChecks.h"
extern "C" uint32_t g_blockMoveChecks;
#endif
#include "mac/CodeRangeCache.h"
#pragma push_macro("Read")
#pragma push_macro("Write")
#undef Read
#undef Write
#include "mac/RectBounds.h"
#pragma pop_macro("Write")
#pragma pop_macro("Read")
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
#include "mac/HandleCopy.h"
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
#ifdef POP2_GAMEPLAY_ROUTE_PROBE
#include "../../../build/gameplay/RouteInputs.h"
#endif
#ifdef POP2_FILE_REPLAY
#include "../../../build/core/FileReplay.h"
#endif
extern DosLibrary* DOSBase;
extern "C" {
// Shared directly with the native guard; there is only one owner record.
uint32_t displayBatchFrame=0,displayBatchParent=0,displayBatchReturn=0;
void pop2SaveRectRow();
void pop2_call_mac(void*,void*,void*);
void pop2_timer_callback();
uint32_t pop2WallTicks();
uint32_t pop2ServiceTicks();
extern volatile uint32_t g_clockFault;
#if defined(POP2_CUTSCENE_ATTRACT_PROBE) && (!defined(POP2_VERIFY) || !defined(POP2_INTEGRATION_PROBE))
#error Attract observation requires explicit verification and integration probes
#endif
#ifdef POP2_INTEGRATION_PROBE
void pop2_integration_seed(void*,void*);void pop2_integration_title();
uint32_t g_integrationOpeningReturn=0,g_integrationTitleTarget=0,g_integrationRoomReturn=0,g_integrationDeathTarget=0,g_integrationDeathReturn=0,g_integrationDeathKind=0;
volatile uint32_t g_integrationStage=0,g_integrationRightAt=0;
volatile uint32_t g_integrationTitlePhase=0,g_integrationTitleCue=0;
#ifdef POP2_INTEGRATION_TITLE_STATE
volatile uint32_t g_integrationTitleState=0;
#endif
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
#if defined(POP2_GAMEPLAY_HALL_VIEW_PROBE) && !defined(POP2_GAMEPLAY_RESTART_PROBE)
#error Hall view requires the verified gameplay lifecycle harness
#endif
#ifdef POP2_GAMEPLAY_HALL_VIEW_PROBE
volatile uint32_t g_hallViewPhase=0,g_hallViewOwner=0,g_hallViewOwnerReturn=0;
extern volatile uint32_t g_hallViewKeyRequest;
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
#if defined(POP2_ROUTE_NO_TRAP_TRACE) && !defined(POP2_GAMEPLAY_ROUTE_PROBE)
#error Trap-trace control requires the explicit gameplay route fixture
#endif
#ifdef POP2_GAMEPLAY_ROUTE_PROBE
#if (!defined(POP2_VERIFY) && !defined(POP2_SCENE_PROFILE)) || !defined(POP2_INTEGRATION_PROBE) || defined(POP2_GAMEPLAY_RESTART_PROBE)
#error Route probe requires separate verification and integration input
#endif
uint32_t g_routeReturn=0;
uint32_t g_routeEndingReturn=0;
uint32_t g_routeHallReturn=0;
volatile uint32_t g_routeReserve[4]={};
volatile uint32_t g_routeHallComplete=0;
#ifdef POP2_GAMEPLAY_HALL_PROBE
volatile uint32_t g_routeHallFileCount=0,g_routeHallFiles[16][6]={};
volatile char g_routeHallNames[16][256]={};
#endif
#ifdef POP2_ROUTE_RESOURCE_TRACE
volatile uint32_t g_routeHandleCount=0,g_routeHandles[256][8],g_routeFreedUnlocks=0;
#endif
volatile uint32_t g_routeFrames=0,g_routeCheckpoint=0,g_routeKeys=0,g_routeRooms=0;
volatile uint32_t g_routeLastResult=0,g_routeComplete=0;
// Preserve the observed boundary even when the strict comparison stops execution.
volatile uint32_t g_routeFirstFrame[5]={};
const char* g_routeError=0;
#ifdef POP2_ROUTE_AUDIO_TRACE
volatile uint32_t g_routeAudioCount=0;
uint32_t g_routeAudio[8192][10]={};
#endif
#ifdef POP2_ROUTE_RANDOM_TRACE
volatile uint32_t g_routeRandom[5];
__attribute__((noinline)) void pop2GameplayRouteRandom(){__asm__ volatile("nop" ::: "memory");}
#endif
void pop2_gameplay_route_frame();
void pop2_gameplay_route_ending();
void pop2_gameplay_route_hall();
__attribute__((noinline)) void pop2GameplayRouteCheckpoint(){__asm__ volatile("nop" ::: "memory");}
__attribute__((noinline)) void pop2GameplayRouteRejectedBoundary(){__asm__ volatile("nop" ::: "memory");}
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
#if defined(POP2_SCENE_DEADLINES) && !defined(POP2_SCENE_PROFILE)
#error Cinematic deadlines require the scene profiling harness
#endif
#ifdef POP2_SCENE_PROFILE
#if defined(POP2_VERIFY) || !defined(POP2_TIMING_PROBE) || !defined(POP2_GAMEPLAY_ROUTE_PROBE)
#error Scene measurement requires the explicit release timing and assisted-route probes
#endif
// Buffered timer requests; no debugger stops or hashing within animation loops.
// Caller identity and the original timer selector distinguish independent clocks.
#if POP2_SCENE_SELECTOR != 1 && POP2_SCENE_SELECTOR != 6
#error Unsupported measured cinematic
#endif
uint32_t g_sceneReturn=0;
volatile uint32_t g_sceneActive=0,g_sceneFinished=0,g_sceneCount=0,g_sceneResult=0;
void pop2_scene_return();
__attribute__((noinline)) void pop2SceneBoundary(){__asm__ volatile("nop" ::: "memory");}
#ifdef POP2_SCENE_DEADLINES
void pop2SceneAnimationRestore();
#endif
void pop2SceneReturned(uint32_t result){
#ifdef POP2_SCENE_DEADLINES
    pop2SceneAnimationRestore();
#endif
    g_sceneResult=result&65535;g_sceneActive=0;g_sceneFinished=1;pop2SceneBoundary();
}
volatile uint32_t g_sceneRows[4096][8]={};
volatile uint32_t g_sceneClockCount=0,g_sceneClocks[2048][8]={};
#ifdef POP2_SCENE_DEADLINES
uint32_t g_sceneAnimationMethod=0,g_sceneAnimationReturn=0;
volatile uint32_t g_sceneAnimationCount=0,g_sceneAnimations[512][10]={};
void pop2_scene_animation();void pop2_scene_animation_return();
#endif
#endif
#ifdef POP2_OPENING_PROFILE
#if !defined(POP2_TIMING_PROBE) || !defined(POP2_INTEGRATION_NO_RIGHT) || defined(POP2_VERIFY)
#error Opening profile requires the PROFILE=1 timing probe and the no-input route
#endif
// Opening-sequence cycle profile (item 19): one row per completed frame from
// the first room to the death song, buffered in Fast RAM and read once after
// restoration. Landing and Kid health changes end profiler phases.
volatile uint32_t g_openingCount=0,g_openingPhases=0,g_openingRows[1024][20]={};
__attribute__((noinline)) void pop2OpeningPhase(){__asm__ volatile("nop" ::: "memory");}
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
#ifdef POP2_RESOURCE_CACHE_PROBE
#if !defined(POP2_VERIFY) || !defined(POP2_RESOURCE_PROBE)
#error Cache readback requires the explicit resource verification fixture
#endif
uint16_t g_resourceProbeID=6401;
#else
uint16_t g_resourceProbeID=6400;
#endif
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
void pop2SectRect();void pop2UnionRect();void pop2EmptyRect();
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
static void invalidateQuickOwner();
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
bool vectorsInstalled=false;unsigned vectorFirst=0,vectorEnd=11;
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
bool didStop=false,normalExit=false;
uint8_t auditBuffer[65536];
uint8_t* patchedLoad=0;uint8_t* patchedExit=0;
struct Resource { uint32_t type;int16_t id;OriginalApplication::Resource source;MacHeap::Handle handle;bool detached;uint32_t identity;int16_t fileRef;uint8_t changes; };
Resource resources[1024]={};unsigned resourceCount=0;int16_t resourceError=0;
bool validMemory(uint32_t at,uint32_t n) {
    return (at>=(uint32_t)stack && at-(uint32_t)stack<=65536 && n<=65536-(at-(uint32_t)stack)) ||
        (at>=(uint32_t)world && at-(uint32_t)world<=worldSize && n<=worldSize-(at-(uint32_t)world)) ||
        (at>=(uint32_t)arena && at-(uint32_t)arena<=g_coreZoneBytes && n<=g_coreZoneBytes-(at-(uint32_t)arena));
}
bool fail(const char* why);
#if defined(POP2_SCENE_PROFILE) && defined(POP2_SCENE_DEADLINES)
extern "C" void pop2SceneAnimationRestore() {
    if(g_sceneAnimationMethod)sl(a5-0x2288+0x1c,g_sceneAnimationMethod);
}
uint8_t* sceneAnimationObject=0;
volatile uint32_t* sceneAnimationRow=0;
extern "C" uint32_t pop2SceneAnimationEnter(uint8_t* entry) {
    // This wrapper retains the original RTD ABI and records eligible calls only.
    // No clock calls, debugger stops or changes to the object's scheduling state.
    if(g_sceneAnimationReturn || !validMemory((uint32_t)entry,12)) {
        didStop=true;fail("SCENE DEADLINE / NESTED CALL OR STACK");return 0;
    }
    g_sceneAnimationReturn=l(entry);sl(entry,(uint32_t)pop2_scene_animation_return);
    sceneAnimationRow=0;sceneAnimationObject=(uint8_t*)l(entry+4);
    if(!validMemory((uint32_t)sceneAnimationObject,0x12)) {
        didStop=true;fail("SCENE DEADLINE / OBJECT");return 0;
    }
    uint8_t* object=sceneAnimationObject;uint32_t tick=l(entry+8);
    if(!g_sceneActive || !w(object+8) || (!w(object+10) && tick<l(object+14)))return 1;
    if(g_sceneAnimationCount==512) {didStop=true;fail("SCENE DEADLINE / CAPACITY");return 0;}
    sceneAnimationRow=g_sceneAnimations[g_sceneAnimationCount++];
    sceneAnimationRow[0]=tick;sceneAnimationRow[1]=w(object+10)?tick:l(object+14);
    sceneAnimationRow[2]=(int32_t)(int16_t)w(object+12);sceneAnimationRow[3]=l(object+4);
    sceneAnimationRow[4]=w(object+10);return 1;
}
extern "C" void pop2SceneAnimationReturned(uint32_t result) {
    if(sceneAnimationRow) {
        sceneAnimationRow[5]=l(sceneAnimationObject+14);
        sceneAnimationRow[6]=(int32_t)(int16_t)w(sceneAnimationObject+12);
        sceneAnimationRow[7]=l(sceneAnimationObject+4);
        sceneAnimationRow[8]=result&65535;sceneAnimationRow[9]=g_scaffoldFields;
    }
    sceneAnimationRow=0;
}
#endif

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
__attribute__((noinline)) TimerStamp timerNow() {
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
__attribute__((noinline)) bool timerDueNow(TimerStamp due) {
    int32_t fields=g_scaffoldFields-due.field;
    if(fields)return fields>0;
    uint32_t before,after;uint16_t hi,lo,check;
    do {before=g_scaffoldFields;hi=*vposrPointer;lo=*vhposrPointer;
        check=*vposrPointer;after=g_scaffoldFields;
    } while(before!=after || ((hi^check)&1));
    fields=after-due.field;
    if(fields)return fields>0;
    // Keep the old calculation for an ambiguous horizontal sample. Valid
    // positions use its exact inverse, avoiding division on each readiness poll.
    if((lo&255)>=227)return timerDue(timerNow(),due);
    bool ready=((uint32_t(hi&1)<<16)|lo)>=VideoTicks::deadlinePALBeam(due.micro);
#if defined(POP2_VERIFY) && defined(POP2_INPUT)
    // Audit the inverse against the previous arithmetic on the same snapshot;
    // a second beam read could cross the deadline while verifying it.
    uint32_t us=(((hi&1)<<8)|(lo>>8))*64+((lo&255)*64)/227;
    if(ready!=timerDue({after,us<20000?us:19999},due))g_clockFault=1;
#endif
    return ready;
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
bool fail(const char* why) { normalExit=false;g_coreReason=why;return false; }
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
    if(segment==17 && !SaveRectPatch::install(code,size,(uint32_t)pop2SaveRectRow))
        return fail("RENDER / SAVERECT ORIGINAL BYTE MISMATCH");
    if(segment==17 && !BankSkipPatch::install(code,size))
        return fail("RENDER / BANK SKIP ORIGINAL BYTE MISMATCH");
    flushCode();return true;
}
Resource* byHandle(MacHeap::Handle h) {
    for(unsigned i=0;i<resourceCount;++i)if(resources[i].handle==h)return resources+i;
    return 0;
}
int32_t resourceHandleRead(void* context,uint32_t offset,uint8_t* bytes,uint32_t count,uint32_t& actual) {
    actual=0;auto handle=(MacHeap::Handle)context;
    if(!heap.isHandle(handle) || !*handle)return MacFiles::unsupported;
    uint32_t size=heap.handleSize(handle);
    if(offset>size || count>size-offset)return MacFiles::unsupported;
    copy(bytes,*handle+offset,count);actual=count;return 0;
}
struct ResourceSelection {
    Resource* only;
    static int32_t payload(void* context,uint32_t identity,ResourceForks::Source& source,uint32_t& offset,uint32_t& size) {
        Resource* only=((ResourceSelection*)context)->only;
        for(unsigned i=0;i<resourceCount;++i) {
            Resource& r=resources[i];if(r.identity!=identity || r.detached)continue;
            if(only && only!=&r && (r.changes&2))return ResourceDirectory::omitEntry;
            if((r.changes&1) && (!only || only==&r)) {
                if(!heap.isHandle(r.handle) || !*r.handle)return MacFiles::unsupported;
                size=heap.handleSize(r.handle);source={r.handle,size,resourceHandleRead};offset=0;
            }
            return 0;
        }
        return 0; // Unloaded, unchanged entries retain their disk source.
    }
};
int32_t publishResources(int16_t ref,Resource* only=0) {
    bool dirty=NativeResources::dirty(ref);
    for(unsigned i=0;i<resourceCount;++i)if(resources[i].fileRef==ref && !resources[i].detached && resources[i].changes)dirty=true;
    if(!dirty)return 0;
    ResourceSelection selection={only};
    int32_t result=NativeResources::publish(ref,ResourceSelection::payload,&selection);
    if(!result)for(unsigned i=0;i<resourceCount;++i) {
        Resource& r=resources[i];if(r.fileRef==ref && !r.detached && (!only || only==&r)) {
            r.changes=0;if(r.handle)r.source.size=heap.handleSize(r.handle);
        }
    }
    return result;
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
    Resource& r=resources[index];r.type=type;r.id=id;r.detached=false;r.identity=item.identity;r.fileRef=item.file;r.changes=0;
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
    invalidateQuickOwner();
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
    normalExit=false;didStop=true;g_coreReason=why;g_coreStopTrap=trap;g_coreStopSegment=seg;g_coreStopOffset=off;
    sl(frame+2,(uint32_t)pop2_mac_exit);return (uint32_t)usp;
}
}
static void prepareQuickTraps();
namespace CoreRuntime {
const char* error() { return g_coreReason; }
bool exitedNormally() {
    return normalExit && g_coreVectorRestored==1
#ifdef POP2_VERIFY
        && g_coreHeapOK==1
#endif
        ;
}
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
    prepareQuickTraps();
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
    Disable();for(unsigned i=vectorFirst;i<vectorEnd;++i)vectorTable[vectorIndices[i]]=savedVectors[i];
    vectorsInstalled=false;Enable();return true;
}
void resumeVectors() {
    Disable();for(unsigned i=vectorFirst;i<vectorEnd;++i)vectorTable[vectorIndices[i]]=portVectors[i];
    vectorsInstalled=true;Enable();
}
void execute() {
    const bool whdload=pop2WHDLoadBinding[3]!=0;
    const unsigned long nativeVBR=whdload?0:(unsigned long)AmigaHardware::getVBR();
    vectorTable=(volatile uint32_t*)(MacRuntime::lineAVectorAddress(whdload,nativeVBR)-0x28);
    // Under WHDLoad only Line-A is forwarded. Other faults remain WHDLoad's
    // named exceptions; never write its protected real exception table.
    vectorFirst=whdload?8:0;vectorEnd=whdload?9:11;
    void (*handlers[])()={pop2_exception_2,pop2_exception_3,pop2_exception_4,pop2_exception_5,
        pop2_exception_6,pop2_exception_7,pop2_exception_8,pop2_exception_9,pop2_line_a,pop2_exception_11,pop2_exception_14};
    for(unsigned i=vectorFirst;i<vectorEnd;++i) { savedVectors[i]=vectorTable[vectorIndices[i]];portVectors[i]=(uint32_t)handlers[i]; }
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
    for(unsigned i=vectorFirst;i<vectorEnd;++i)if(vectorTable[vectorIndices[i]]!=savedVectors[i])g_coreVectorRestored=0;
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
#ifdef POP2_GAMEPLAY_ROUTE_PROBE
static uint8_t routeHeld=0;
static bool routeKeyboard(uint16_t step){
    if(g_routeKeys>=sizeof(routeKeys)/sizeof(routeKeys[0]) || routeKeys[g_routeKeys].step!=step)
        return fail("GAMEPLAY ROUTE / KEY STEP");
    const uint8_t raw[]={0x4f,0x4e,0x4c,0x4d,0x60,0x63,0x40};
    uint8_t held=routeKeys[g_routeKeys].mask,changed=held^routeHeld;
    for(unsigned i=0;i<sizeof(raw);++i)if(changed&(1<<i))NativeInput::inject(raw[i],(held&(1<<i))!=0);
    if(changed){uint32_t tick=NativeInput::ticks();while(NativeInput::ticks()==tick){}}
    routeHeld=held;
    if(NativeInput::error())return fail(NativeInput::error());
    return true;
}
static void routeCapture(unsigned ordinal){
    g_routeCheckpoint=ordinal;pop2GameplayRouteCheckpoint();
#if POP2_GAMEPLAY_ROUTE_LEVEL == 0
    // Explicit verification-only interventions at the same stopped boundary as
    // the reference. Capture before and after; reject unexpected prior state.
    while(ordinal+1<sizeof(routeCheckpoints)/sizeof(routeCheckpoints[0]) &&
          routeCheckpoints[ordinal+1].automatic){
        ++ordinal;
        const auto& point=routeCheckpoints[ordinal];
        if(point.step!=g_routeFrames || point.keys!=g_routeKeys){
            g_routeError="GAMEPLAY ASSIST / BOUNDARY";return;
        }
        for(const auto& write:routeAssists)if(write.ordinal==ordinal){
            if(w(a5-write.offset)!=write.before){g_routeError="GAMEPLAY ASSIST / BEFORE VALUE";return;}
        }
        for(const auto& write:routeAssists)if(write.ordinal==ordinal){
            sw(a5-write.offset,write.after);
            if(w(a5-write.offset)!=write.after){g_routeError="GAMEPLAY ASSIST / WRITE";return;}
        }
        if(point.level!=w(a5-0x4fd2) || point.room!=w(a5-0x4fc2)){
            g_routeError="GAMEPLAY ASSIST / AFTER BOUNDARY";return;
        }
        g_routeCheckpoint=ordinal;pop2GameplayRouteCheckpoint();
    }
#endif
    if(ordinal+1==sizeof(routeCheckpoints)/sizeof(routeCheckpoints[0]))g_routeComplete=1;
}
extern "C" void pop2GameplayRouteFrame(uint32_t result){
    ++g_routeFrames;g_routeLastResult=result&65535;
    // The guarded FrameAdv owner has returned. Finish its one publication
    // before observing AGA output; clear ownership to avoid a second publish.
    if(displayBatchFrame){
        displayBatchFrame=displayBatchParent=displayBatchReturn=0;
        NativeDisplay::paceFrame();
        if(!NativeDisplay::publish(displayBatchScene)){g_routeError=NativeDisplay::error();return;}
        displayBatchScene=false;
    }
    if(g_routeFrames!=w(a5-0x349a)){g_routeError="GAMEPLAY ROUTE / FRAME STEP";return;}
    unsigned next=g_routeCheckpoint+1;
    if(next<sizeof(routeCheckpoints)/sizeof(routeCheckpoints[0]) &&
       int(next)!=routeEndingOrdinal &&
       !routeCheckpoints[next].first && g_routeFrames==routeCheckpoints[next].step){
        auto& expected=routeCheckpoints[next];
        if((result&65535)!=0xfffe || w(a5-0x4fd2)!=expected.level ||
           w(a5-0x4fc2)!=expected.room || g_routeKeys!=expected.keys){
            g_routeError="GAMEPLAY ROUTE / FRAME RESULT";return;
        }
        NativeDisplay::wait();routeCapture(next);
    }
}
extern "C" void pop2GameplayRouteHall(){g_routeHallComplete=1;}
extern "C" void pop2GameplayRouteEnding(){
    unsigned next=g_routeCheckpoint+1;
    if(int(next)!=routeEndingOrdinal || next>=sizeof(routeCheckpoints)/sizeof(routeCheckpoints[0])){
        g_routeError="GAMEPLAY ENDING / CHECKPOINT ORDER";return;
    }
    const auto& expected=routeCheckpoints[next];
    if(g_routeFrames!=expected.step || g_routeKeys!=expected.keys ||
       w(a5-0x4fd2)!=expected.level || w(a5-0x4fc2)!=expected.room){
        g_routeError="GAMEPLAY ENDING / STATE";return;
    }
    NativeDisplay::wait();routeCapture(next);
}
#endif
#ifdef POP2_RECTANGLE_PROBE
extern "C" {
void pop2RectProbe();void pop2RectProbeEnd();
void pop2RectTrapSect();void pop2RectTrapUnion();void pop2RectTrapEmpty();
uint32_t g_rectEntry,g_rectArgs[3],g_rectRegisters[15],g_rectBeforeSP,g_rectAfterSP;
uint16_t g_rectTrap,g_rectInitialCCR,g_rectCCR,g_rectResult;
uint8_t g_rectRows[1220][92];uint32_t g_rectCount=0,g_rectGuards=0,g_rectTrapCalls=0;
int32_t pop2PureRect(uint8_t*,uint32_t*,uint32_t);
__attribute__((noinline)) void pop2RectProbeDone(){__asm__ volatile("nop" ::: "memory");}
}
static void rectangleFixture(){
    static const int16_t rectangles[][4]={{0,0,10,20},{5,10,15,30},{10,20,20,40},{0,0,0,0},
        {10,20,0,0},{-32768,-32768,32767,32767},{-32768,-1,0,1},{0,-32768,1,32767},
        {-20,-30,-10,-5},{32767,32767,-32768,-32768}};
    const uint16_t traps[]={0xa8aa,0xa8ab,0xa8ae};
#ifdef POP2_RECTANGLE_TRAP_PROBE
    void (*entries[])()={pop2RectTrapSect,pop2RectTrapUnion,pop2RectTrapEmpty};
#else
    void (*entries[])()={pop2SectRect,pop2UnionRect,pop2EmptyRect};
#endif
    for(unsigned t=0;t<3;++t)for(unsigned a=0;a<10;++a)for(unsigned b=0;b<(t==2?1:10);++b)
    for(unsigned alias=0;alias<(t==2?1:3);++alias)for(unsigned cc=0;cc<2;++cc){
        uint8_t rects[24];for(unsigned i=0;i<4;++i){sw(rects+i*2,rectangles[a][i]);sw(rects+8+i*2,rectangles[b][i]);}
        const uint16_t sentinel[]={123,456,789,1234};for(unsigned i=0;i<4;++i)sw(rects+16+i*2,sentinel[i]);
        g_rectEntry=(uint32_t)entries[t];g_rectTrap=traps[t];g_rectInitialCCR=cc?31:0;
        g_rectArgs[0]=(uint32_t)(rects+alias*8);g_rectArgs[1]=(uint32_t)(rects+8);g_rectArgs[2]=(uint32_t)rects;
        pop2RectProbe();uint8_t* row=g_rectRows[g_rectCount++];
        sw(row,g_rectCCR&31);sw(row+2,g_rectResult);__builtin_memcpy(row+4,rects,24);
        for(unsigned i=0;i<15;++i)sl(row+28+i*4,g_rectRegisters[i]);
        sl(row+88,g_rectAfterSP-g_rectBeforeSP);
    }
    // Same production guard, with each failed precondition independently active.
    for(unsigned t=0;t<3;++t)for(unsigned fault=0;fault<(t==2?3:5);++fault){
        uint8_t rect[8]={},args[18]={};uint32_t saved[2]={0x12345678,0x23456789};
        sl(args,(uint32_t)pop2RectProbeEnd-2);
        sl(args+4,(uint32_t)rect);sl(args+8,(uint32_t)rect);sl(args+12,(uint32_t)rect);
        uint8_t* input=args+4;
        if(fault==0)input=stack+65536-2;
        else if(fault==1)sl(args,0);
        else sl(args+(t==2?4:fault==2?12:fault==3?8:4),0);
        if(pop2PureRect(input,saved,traps[t])==-1 && saved[0]==0x12345678 && saved[1]==0x23456789)++g_rectGuards;
    }
    pop2RectProbeDone();
}
#endif
#ifdef POP2_QUICK_TRAP_PROBE
static void quickTrapFixture();
#endif
extern "C" void pop2IntegrationRoomHook(){
#ifdef POP2_RECTANGLE_PROBE
    rectangleFixture();
#endif
    g_integrationPulseRaw=128;NativeInput::flush(0xffff,0);NativeDisplay::wait();
#ifdef POP2_GAMEPLAY_ROUTE_PROBE
    ++g_routeRooms;
    g_routeFirstFrame[0]=g_routeFrames;g_routeFirstFrame[1]=g_routeKeys;
    g_routeFirstFrame[2]=w(a5-0x4fd2);g_routeFirstFrame[3]=w(a5-0x4fc2);
    g_routeFirstFrame[4]=w(a5-0x349a);
    unsigned ordinal=g_routeRooms==1?0:g_routeCheckpoint+1;
    if(ordinal>=sizeof(routeCheckpoints)/sizeof(routeCheckpoints[0])){g_routeError="GAMEPLAY ROUTE / FIRST FRAME COUNT";return;}
    auto& expected=routeCheckpoints[ordinal];
    if(!expected.first || g_routeFrames!=expected.step || w(a5-0x4fd2)!=expected.level ||
       w(a5-0x4fc2)!=expected.room || g_routeKeys!=expected.keys){
        g_routeError="GAMEPLAY ROUTE / FIRST FRAME";
        // Observe the rejected entry before teardown without accepting it or
        // applying the next checkpoint's assistance.
        pop2GameplayRouteRejectedBoundary();return;
    }
    if(g_routeRooms==1){g_integrationStage=3;recordIntegrationStage();}
    routeCapture(ordinal);
#elif defined(POP2_GAMEPLAY_RESTART_PROBE)
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
#ifdef POP2_GAMEPLAY_HALL_VIEW_PROBE
        integrationPress(0x25,60);
#else
        integrationPress(0x13,60);
#endif
#endif
    }
    recordIntegrationStage();
#else
    g_integrationRightAt=NativeInput::ticks()+120;g_integrationStage=3;recordIntegrationStage();
#endif
#ifdef POP2_QUICK_TRAP_PROBE
    quickTrapFixture();
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
#ifndef POP2_GAMEPLAY_IMPORT_PROBE
    if(step==12 && !(g_gameplayInputActions&1)){
        gameplayEdges(0x66,true,0x21,true);g_gameplayInputActions|=1;
    }
#endif
    if(step==23 && !(g_gameplayInputActions&2)){
        gameplayEdges(0x66,true);g_gameplayInputActions|=2;
    }
#ifndef POP2_GAMEPLAY_IMPORT_PROBE
    if(step==24 && !(g_gameplayInputActions&4)){
        gameplayEdges(0x18,true);gameplayEdges(0x18,false,0x66,false);g_gameplayInputActions|=4;
    }
#endif
}
extern "C" void pop2GameplayPickerShown(){integrationPress(0x44,120);}
extern "C" void pop2GameplaySaved(uint32_t result){
    g_gameplaySaveResult=result&65535;g_gameplaySavedAt=NativeInput::ticks();g_integrationPulseRaw=128;
    pop2GameplayFileCheckpoint();
}
extern "C" void pop2GameplayLoaded(uint32_t result){g_gameplayLoadResult=result&65535;pop2GameplayFileCheckpoint();}
#endif
extern "C" void pop2IntegrationDeathHook(uint8_t* originalSP){
    if(g_integrationStage==3 && w(originalSP+4)==3 && !g_integrationDeathReturn
#ifdef POP2_GAMEPLAY_ROUTE_PROBE
       && false
#endif
       ){
        g_integrationDeathKind=w(originalSP+4);g_integrationDeathReturn=l(originalSP);
        sl(originalSP,(uint32_t)pop2_integration_death_return);
    }
}
extern "C" void pop2IntegrationDeathReturned(){
    NativeDisplay::wait();g_integrationStage=4;recordIntegrationStage();
}
extern "C" void pop2IntegrationTitleHook(uint8_t* originalSP){
    uint16_t phase=w(originalSP+4);
#ifdef POP2_INTEGRATION_TITLE_STATE
    // Same guarded renderer fixture as mac_reference.lua; never ordinary play.
    if(phase==15){
        if(g_integrationTitleState || w(a5-0x1f6e)!=100)g_integrationTitleState=2;
        else {sw(a5-0x1f6e,101);g_integrationTitleState=1;}
    }
#endif
    g_integrationTitlePhase=phase;g_integrationTitleCue=w(a5-0x1f6e);
    if(phase==2 && g_integrationStage==0){
        g_integrationPulseRaw=128;g_integrationStage=1;recordIntegrationStage();
    }
    // CODE 15+70ca/70cc changes cue 101 to 114 while selecting the logo.
    // At the next callback the original frame has returned. Its frame number
    // depends on service cost, so schedule test input from the reached cue.
    // The result checker still compares the captured reference pixels.
    if(phase>=3 && w(a5-0x1f6e)==0x72 && g_integrationStage==1){
#ifndef POP2_CUTSCENE_ATTRACT_PROBE
        integrationPress(0x40,120);
#endif
        g_integrationStage=2;recordIntegrationStage();
    }
}
#endif
// Pascal entry adapter for the shared pure rectangle algorithms. Failed guards
// retry the untouched full veneer, retaining its named errors and patch policy.
// No tick, callback, port or framebuffer state is changed by these services.
template<uint16_t trap,bool callable=true>
__attribute__((always_inline)) inline int32_t pureRect(uint8_t* args,uint32_t* saved) {
    uint32_t count=trap==0xa8ae?6:trap==0xa8aa?14:12;
    uint32_t start=(uint32_t)args-(callable?4:0),seg=0,off=0;
    if(start<(uint32_t)stack || start-(uint32_t)stack>65536-count-(callable?4:0))return -1;
    bool owner=!callable || location(l(args-4),seg,off);
#ifdef POP2_RECTANGLE_PROBE
    owner=owner || (l(args-4)>=(uint32_t)pop2RectProbe && l(args-4)<(uint32_t)pop2RectProbeEnd);
#endif
    if(!owner)return -1;
    uint8_t* a=(uint8_t*)l(args+(trap==0xa8ae?0:8));
    if(!validMemory((uint32_t)a,8))return -1;
    if(trap==0xa8ae){
        bool empty=int16_t(w(a))>=int16_t(w(a+4)) || int16_t(w(a+2))>=int16_t(w(a+6));
        args[4]=empty?1:0;
        saved[0]=(saved[0]&0xffff0000UL)|w(a);
        saved[1]=(saved[1]&0xffff0000UL)|w(a+2);
        return empty?0x10:0x14;
    }
    uint8_t* b=(uint8_t*)l(args+4);uint8_t* out=(uint8_t*)l(args);
    if(!validMemory((uint32_t)b,8) || !validMemory((uint32_t)out,8))return -1;
    if(trap==0xa8aa){
        bool nonempty;RectBounds::intersect<BigEndianMemory::read16,BigEndianMemory::write16>(out,a,b,nonempty);args[12]=nonempty?1:0;
        saved[0]=(saved[0]&0xffff0000UL)|14;
        return nonempty?0x11:4;
    }
    RectBounds::unite<BigEndianMemory::read16,BigEndianMemory::write16>(out,a,b);saved[0]=l(out);saved[1]=l(out+4);
    return 0x10|(saved[1]==0?4:(saved[1]&0x80000000UL)?8:0);
}
// Compile each fixed service separately so no runtime selector/temporary loop
// is paid by the Pascal entry; all geometry still uses shared RectBounds.
extern "C" __attribute__((optimize("O3"))) int32_t pop2SectRectBody(uint8_t* args,uint32_t* saved){return pureRect<0xa8aa>(args,saved);}
extern "C" __attribute__((optimize("O3"))) int32_t pop2UnionRectBody(uint8_t* args,uint32_t* saved){return pureRect<0xa8ab>(args,saved);}
extern "C" __attribute__((optimize("O3"))) int32_t pop2EmptyRectBody(uint8_t* args,uint32_t* saved){return pureRect<0xa8ae>(args,saved);}
#ifdef POP2_RECTANGLE_PROBE
extern "C" int32_t pop2PureRect(uint8_t* args,uint32_t* saved,uint32_t trap){
    return trap==0xa8aa?pop2SectRectBody(args,saved):trap==0xa8ab?pop2UnionRectBody(args,saved):pop2EmptyRectBody(args,saved);
}
#endif
// Pure, nonblocking services shared by the ordinary and indexed entries.
// Errors leave the existing named-stop path in charge.
template<uint16_t trap>
static inline bool trivialService(uint32_t* regs) {
    // As in Alone in the Dark, zone services can use a specialized entry.
    // This port has one supported zone; retain the existing unknown-zone stop.
    if(trap==0xa11a){regs[8]=(uint32_t)arena;regs[0]=0;}
    if(trap==0xa01b){
        if(regs[8]!=(uint32_t)arena)return fail("HEAP / UNKNOWN ZONE");
        regs[0]=0;
    }
    if(trap==0xa9b4)regs[0]=(uint32_t)a5;
    if(trap==0xa055)regs[0]&=0x00ffffff;
    if(trap==0xa05d) {
        if((regs[0]&255)>1)return fail("MMU / ADDRESS MODE");
        uint8_t requested=regs[0];regs[0]=addressMode;
        addressMode=requested;low[0xcb2]=requested;
    }
    if(trap==0xa02a) {
        auto h=(MacHeap::Handle)regs[8];
        // A miss leaves all guest state untouched for the full caller policy.
        if(!heap.isHandle(h))return false;
        regs[0]=(int32_t)heap.setState(h,heap.state(h)&~0x80);
    }
    if(trap==0xa029 || trap==0xa069) {
        auto h=(MacHeap::Handle)regs[8];
        if(trap==0xa029)regs[0]=(int32_t)heap.lockHandle(h);
        else regs[0]=heap.state(h);
        if(heap.error())return fail(trap==0xa029 ? "HEAP / UNKNOWN HANDLE" : "HEAP / STATE HANDLE OWNER");
    }
    return true;
}
extern "C" {
uint32_t g_quickServicesValid=0,g_quickServicesTick=0,g_quickServicesField=0;
uint32_t g_quickServicesMicro=0xffffffffUL;
uint32_t g_quickCodeBase=0,g_quickCodeEnd=0,g_quickStackLow=0,g_quickStackHigh=0;
const uint32_t* g_quickCodeIndexes=nullptr;
const uint32_t* g_quickOwnerRows[7]={};
uint8_t** g_quickA5=&a5;
}
static void invalidateQuickOwner() {g_quickCodeBase=0xffffffffUL;g_quickCodeEnd=0;}
#define quickServicesValid g_quickServicesValid
#define quickServicesTick g_quickServicesTick
#define quickServicesField g_quickServicesField
#ifdef POP2_QUICK_TRAP_PROBE
// Isolate only the synthetic fixtures' audio readiness. The real audio clock,
// queues and engine remain untouched and resume normal service afterward.
static int quickFixtureAudioPending=-1;
#endif
#ifdef POP2_AUDIO
__attribute__((always_inline)) static inline bool quickAudioPending() {
#ifdef POP2_QUICK_TRAP_PROBE
    if(quickFixtureAudioPending>=0)return quickFixtureAudioPending!=0;
#endif
    return NativeAudio::servicePending();
}
#endif
template<bool exact>
__attribute__((always_inline)) static inline bool quickServicesReady() {
#if defined(POP2_INPUT) && defined(POP2_UI) && defined(POP2_AUDIO)
    uint32_t tick=exact ? wallTicks() : pop2ServiceTicks(),field=g_scaffoldFields;
    if(g_clockFault)return false;
    if(quickServicesValid && quickServicesTick==tick && quickServicesField==field)
        return g_quickServicesMicro==0xffffffffUL || !timerDueNow({field,g_quickServicesMicro});
    quickServicesValid=false;
    if(NativeInput::error() || quickAudioPending() ||
       (vblCount && vblPass.needed(tick)))return false;
    // A future field cannot have a due microsecond timer. A timer in the
    // current field takes the exact existing timerNow path, even if early.
    uint32_t micro=0xffffffffUL;
    for(unsigned i=0;i<timerCount;++i)if(timers[i].active && int32_t(timers[i].due.field-field)<=0) {
        if(timerDueNow(timers[i].due))return false;
        if(timers[i].due.micro<micro)micro=timers[i].due.micro;
    }
    quickServicesTick=tick;quickServicesField=field;
    // A current-field certificate retains the earliest precise deadline;
    // every native reuse checks it. Never reuse across an interrupt arriving
    // during validation. The full path invalidates before mutating queues.
    g_quickServicesMicro=micro;
    quickServicesValid=field==g_scaffoldFields;
    return field==g_scaffoldFields;
#else
    return false;
#endif
}
// A service failure is terminal, not a guard miss. Preserve its identity for
// the user-mode loud-stop path rather than executing a failed body twice.
struct QuickDisplayFailure {const char* reason;uint32_t pc,usp;uint16_t trap;};
static QuickDisplayFailure quickDisplayFailure={};
static const char quickDisplayOwnerError[]="DISPATCH / QUICK FAILURE OWNER";
using QuickTrap=uint32_t (*)(uint32_t*,uint8_t*,uint8_t*);
extern "C" { QuickTrap g_pop2QuickTraps[4096]={}; }
template<uint16_t trap>
__attribute__((optimize("O3"))) static uint32_t quickTrap(uint32_t* regs,uint8_t* frame,uint8_t* usp) {
#if defined(POP2_INPUT) && defined(POP2_DISPLAY) && defined(POP2_UI) && defined(POP2_AUDIO)
    // Adopt Alone in the Dark's unfinished-frame / no-due-service guard.
    // TickCount samples precisely; other services use the interrupt epoch or
    // the latest precise observation, as does the ordinary VBL selector.
    // Any miss retains the complete ordinary path, before changing game state.
    if((w(frame)&0x2000) || didStop || g_timerCallbackActive)return 0;
    if(usp<stack+128 || usp>stack+65536-4)return 0;
    constexpr bool displayQuick=trap==0xa8a8 || trap==0xa8a9 || trap==0xa8a7 ||
        trap==0xa8aa || trap==0xa8ab || trap==0xa8ae || trap==0xaa14 ||
        trap==0xaa15 || trap==0xab1d;
    if(displayQuick && !displayBatchFrame)return 0;
    if(trap==0xab1d && (regs[0]!=0x0004000f || usp>stack+65536-8))return 0;
    if(displayBatchFrame && ((uint32_t)usp>displayBatchFrame ||
       l((uint8_t*)displayBatchFrame)!=displayBatchParent ||
       l((uint8_t*)displayBatchFrame+4)!=displayBatchReturn))return 0;
    uint32_t seg,off,pc=l(frame+2);
    if(!location(pc,seg,off))return 0;
#if defined(POP2_TIMING_PROBE) && defined(POP2_INTEGRATION_PROBE)
    // Retain the existing SetTimer request recorder on its original path.
    if(trap==0xa975 && seg==17 && off==0x313e)return 0;
#ifdef POP2_SCENE_PROFILE
    if(trap==0xa975 && seg==15 && (off==0x3152 || off==0x321a || off==0x3984 || off==0x3b72))return 0;
#ifdef POP2_SCENE_DEADLINES
    if(trap==0xa975 && seg==16 && l(a5-0x2288+0x1c)!=(uint32_t)pop2_scene_animation && (off==0x128c || off==0x1358))return 0;
#endif
#endif
#endif
    if(!quickServicesReady<trap==0xa975>())return 0;
    if(displayQuick) {
        uint32_t consumed=0;
        if(trap==0xa8aa || trap==0xa8ab || trap==0xa8ae) {
            int32_t cc=pureRect<trap,false>(usp,regs);
            if(cc<0)return 0;
            consumed=trap==0xa8ae?4:12;sw(frame,(w(frame)&~31)|cc);
        } else if(!NativeDisplay::dispatch(trap,regs,usp,consumed)) {
            quickDisplayFailure={NativeDisplay::error(),pc,(uint32_t)usp,trap};
            return 0;
        }
        return consumed+1;
    }
    if(!trivialService<trap>(regs))return 0;
    g_quickCodeBase=g_coreSegments[seg][0];g_quickCodeEnd=g_quickCodeBase+g_coreSegments[seg][1]-2;
    if(trap==0xa975){sl(usp,quickServicesTick);regs[0]=0;}
    if(!(trap&0x0800)) {
        sw(low+0x220,heap.error());
        uint16_t sr=w(frame)&~15,result=regs[0];
        if(!result)sr|=4;if(result&0x8000)sr|=8;sw(frame,sr);
    }
#if defined(POP2_VERIFY) && !defined(POP2_ROUTE_NO_TRAP_TRACE)
    uint32_t* row=g_coreTrace[(g_coreCount++)%traceCapacity];row[0]=seg;row[1]=off;row[2]=trap;
#endif
    return 1;
#else
    return 0;
#endif
}
extern "C" {
void pop2QuickSystemTask();void pop2QuickTickCount();
void pop2QuickStripAddress();void pop2QuickSwapMMUMode();
void pop2QuickHLock();void pop2QuickHGetState();void pop2QuickHUnlock();
uint32_t pop2QuickHUnlockBody(uint32_t*,uint8_t*,uint8_t*);
}
static void prepareQuickTraps() {
#ifdef POP2_QUICK_TRAP_PROBE
    // Run the exhaustive copy oracle before starting game clocks/input.
    g_blockMoveChecks=BlockMoveChecks::run(BlockMoveBytes::move);
#endif
    static_assert(CodeRangeCache::slots==256,"native code-range index size");
    invalidateQuickOwner();
    g_quickCodeIndexes=codeRanges.indexes();
    for(auto& row:g_quickOwnerRows)row=nullptr;
    g_quickStackLow=(uint32_t)stack+128;g_quickStackHigh=(uint32_t)stack+65536-4;
#if defined(POP2_INPUT) && defined(POP2_DISPLAY) && defined(POP2_UI) && defined(POP2_AUDIO)
    g_pop2QuickTraps[0x975]=(QuickTrap)((uint32_t)pop2QuickTickCount|1);
    g_pop2QuickTraps[0x9b4]=(QuickTrap)((uint32_t)pop2QuickSystemTask|1);
    g_pop2QuickTraps[0x055]=(QuickTrap)((uint32_t)pop2QuickStripAddress|1);
    g_pop2QuickTraps[0x05d]=(QuickTrap)((uint32_t)pop2QuickSwapMMUMode|1);
    g_pop2QuickTraps[0x029]=(QuickTrap)((uint32_t)pop2QuickHLock|1);
    g_pop2QuickTraps[0x069]=(QuickTrap)((uint32_t)pop2QuickHGetState|1);
    g_pop2QuickTraps[0x02a]=(QuickTrap)((uint32_t)pop2QuickHUnlock|1);
    g_pop2QuickTraps[0x8a8]=quickTrap<0xa8a8>;
    g_pop2QuickTraps[0x8a9]=quickTrap<0xa8a9>;
    g_pop2QuickTraps[0x8a7]=quickTrap<0xa8a7>;
    g_pop2QuickTraps[0x8aa]=quickTrap<0xa8aa>;
    g_pop2QuickTraps[0x8ab]=quickTrap<0xa8ab>;
    g_pop2QuickTraps[0x8ae]=quickTrap<0xa8ae>;
    g_pop2QuickTraps[0xa14]=quickTrap<0xaa14>;
    g_pop2QuickTraps[0xa15]=quickTrap<0xaa15>;
    g_pop2QuickTraps[0xb1d]=quickTrap<0xab1d>;
#endif
}
extern "C" uint32_t pop2QuickStripAddressBody(uint32_t* r,uint8_t* f,uint8_t* u){return quickTrap<0xa055>(r,f,u);}
extern "C" uint32_t pop2QuickSwapMMUModeBody(uint32_t* r,uint8_t* f,uint8_t* u){return quickTrap<0xa05d>(r,f,u);}
extern "C" uint32_t pop2QuickHLockBody(uint32_t* r,uint8_t* f,uint8_t* u){return quickTrap<0xa029>(r,f,u);}
extern "C" uint32_t pop2QuickHUnlockBody(uint32_t* r,uint8_t* f,uint8_t* u){return quickTrap<0xa02a>(r,f,u);}
extern "C" uint32_t pop2QuickHGetStateBody(uint32_t* r,uint8_t* f,uint8_t* u){return quickTrap<0xa069>(r,f,u);}
// Reuse the shared current-range validator on a native owner candidate miss;
// changing CODE owners need not repeat the other guard checks in a C wrapper.
extern "C" __attribute__((optimize("O3"))) uint32_t pop2QuickLocate(uint32_t pc) {
    uint32_t seg,off;
    if(!location(pc,seg,off))return 0;
    g_quickCodeBase=g_coreSegments[seg][0];
    g_quickCodeEnd=g_quickCodeBase+g_coreSegments[seg][1]-2;
    return (uint32_t)g_coreSegments[seg];
}
// Called only after the native stack, CODE and frame-owner checks. Refresh the
// same certificate used by the checked C entries; pending work still falls
// back before any service result or game state is changed.
extern "C" __attribute__((optimize("O3"))) uint32_t pop2QuickReady(uint32_t kind) {
    if(didStop || g_timerCallbackActive)return 0;
    return kind==1 ? quickServicesReady<true>() : quickServicesReady<false>();
}
extern "C" uint32_t pop2QuickDeadlineReady() {
    return !timerDueNow({g_quickServicesField,g_quickServicesMicro});
}
// The native guard has already validated the caller and no-due certificate.
// Share the service bodies and OS result/CCR policy with the full dispatcher.
template<uint16_t trap>
__attribute__((always_inline)) static inline uint32_t quickTrivialBody(uint32_t* regs,uint8_t* frame) {
    uint32_t saved=regs[0];
    if(!trivialService<trap>(regs)){regs[0]=saved;return 0;}
    sw(low+0x220,heap.error());
    uint16_t sr=w(frame)&~15,result=regs[0];
    if(!result)sr|=4;if(result&0x8000)sr|=8;sw(frame,sr);
    return 1;
}
extern "C" __attribute__((optimize("O3"))) uint32_t pop2QuickStripAddressWarm(uint32_t* r,uint8_t* f){return quickTrivialBody<0xa055>(r,f);}
extern "C" __attribute__((optimize("O3"))) uint32_t pop2QuickSwapMMUModeWarm(uint32_t* r,uint8_t* f){return quickTrivialBody<0xa05d>(r,f);}
extern "C" __attribute__((optimize("O3"))) uint32_t pop2QuickHUnlockWarm(uint32_t* r,uint8_t* f){return quickTrivialBody<0xa02a>(r,f);}
extern "C" __attribute__((optimize("O3"))) uint32_t pop2QuickHLockWarm(uint32_t* r,uint8_t* f){return quickTrivialBody<0xa029>(r,f);}
extern "C" __attribute__((optimize("O3"))) uint32_t pop2QuickHGetStateWarm(uint32_t* r,uint8_t* f){return quickTrivialBody<0xa069>(r,f);}
extern "C" uint32_t pop2QuickTickCountBody(uint32_t* r,uint8_t* f,uint8_t* u){return quickTrap<0xa975>(r,f,u);}
extern "C" uint32_t pop2QuickSystemTaskBody(uint32_t* r,uint8_t* f,uint8_t* u){return quickTrap<0xa9b4>(r,f,u);}
#ifdef POP2_VERIFY
extern "C" {uint32_t g_quickGuardChecks=0,g_quickGuardFault=0;}
extern "C" uint32_t pop2QuickVerify(uint32_t* regs,uint8_t* frame,uint8_t* usp) {
    uint32_t seg=0,off=0,pc=l(frame+2);
    bool valid=location(pc,seg,off) && usp>=stack+128 && usp<=stack+65536-4;
    valid=valid && (!displayBatchFrame || ((uint32_t)usp<=displayBatchFrame &&
        l((uint8_t*)displayBatchFrame)==displayBatchParent &&
        l((uint8_t*)displayBatchFrame+4)==displayBatchReturn));
    valid=valid && !(w(frame)&0x2000) && !didStop && !g_timerCallbackActive;
#ifdef POP2_INPUT
    valid=valid && !g_clockFault;
#endif
#if defined(POP2_INPUT) && defined(POP2_UI) && defined(POP2_AUDIO)
    uint32_t field=g_scaffoldFields;
    if(field==quickServicesField) {
        valid=valid && !NativeInput::error() && !NativeAudio::servicePending() &&
            (!vblCount || !vblPass.needed(quickServicesTick));
        // Reusable certificates must cover every active deadline. The native
        // entry checks the earliest deadline before accepting; timerDueNow
        // audits that comparison on the same snapshot in VERIFY builds.
        if(quickServicesValid)for(unsigned i=0;i<timerCount;++i)
            if(timers[i].active) {
                int32_t delta=timers[i].due.field-field;
                if(delta<0 || (!delta && timers[i].due.micro<g_quickServicesMicro))valid=false;
            }
        if(field==g_scaffoldFields){++g_quickGuardChecks;if(!valid)g_quickGuardFault=1;}
    }
#endif
#ifndef POP2_ROUTE_NO_TRAP_TRACE
    uint32_t* row=g_coreTrace[(g_coreCount++)%traceCapacity];row[0]=seg;row[1]=off;row[2]=w((uint8_t*)pc);
#endif
    (void)regs;return !g_quickGuardFault;
}
#endif
static uint32_t callbackService(uint32_t* parked);
extern "C" uint32_t pop2Dispatch(uint32_t*,uint8_t*,uint8_t*);
#include "QuickTrapProbe.inc"
extern "C" uint32_t pop2Dispatch(uint32_t* regs,uint8_t* frame,uint8_t* usp) {
    uint32_t pc=l(frame+2),seg=0,off=0;
    if(quickDisplayFailure.reason) {
        QuickDisplayFailure failure=quickDisplayFailure;quickDisplayFailure={};
        if(pc!=failure.pc || (uint32_t)usp!=failure.usp || !location(pc,seg,off))
            return stop(frame,usp,quickDisplayOwnerError,failure.trap,seg,off);
        return stop(frame,usp,failure.reason,failure.trap,seg,off);
    }
#ifdef POP2_VERIFY
    if(g_quickGuardFault)return stop(frame,usp,"DISPATCH / QUICK GUARD MISMATCH",0,seg,off);
#endif
#ifdef POP2_INTEGRATION_TITLE_STATE
    if(g_integrationTitleState==2)return stop(frame,usp,"INTEGRATION / TITLE STATE INITIAL CUE",0,seg,off);
#endif
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
    // Original CODE allocations cannot overlap the linked native fixtures.
    // Resolve uncommon native callers only after the common owner succeeds/fails.
    bool fixtureCaller=!codeCaller && ((pc>=(uint32_t)pop2_audio_lookup && pc<(uint32_t)pop2_audio_fixture_end) || (pc>=(uint32_t)pop2_ui_fixture && pc<(uint32_t)pop2_ui_fixture_end) || (pc>=(uint32_t)pop2_display_scene_init && pc<(uint32_t)pop2_display_scene_end) || (pc>=(uint32_t)pop2_display_fixture && pc<(uint32_t)pop2_display_fixture_end) || (pc>=(uint32_t)pop2_identity_fixture && pc<(uint32_t)pop2_identity_fixture_end) ||
        (pc>=(uint32_t)pop2_file_fixture && pc<(uint32_t)pop2_file_fixture_end) ||
        (pc>=(uint32_t)pop2_replay_fixture && pc<(uint32_t)pop2_replay_fixture_end) ||
        (pc>=(uint32_t)pop2_resource_fixture && pc<(uint32_t)pop2_resource_fixture_end));
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
#ifdef POP2_RECTANGLE_TRAP_PROBE
    if(pc>=(uint32_t)pop2RectProbe && pc<(uint32_t)pop2RectProbeEnd){fixtureCaller=true;++g_rectTrapCalls;}
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
    if(trap==0xaff9) {
        // Original resetLoop writes channel->userInfo even when channel is
        // null. Address $20 belongs to Macintosh low memory, not Exec.
        if(!codeCaller || seg!=20 || off!=0x46b4 || w((uint8_t*)pc+2)!=0x4e71)
            return stop(frame,usp,"LOW MEMORY / INDIRECT STORE OWNER",trap,seg,off);
        uint8_t* destination=low+0x20;
        if(regs[8]) {
            if(regs[8]>0xffffffdfUL || !validMemory(regs[8]+0x20,4))
                return stop(frame,usp,"LOW MEMORY / INDIRECT STORE EXTENT",trap,seg,off);
            destination=(uint8_t*)(regs[8]+0x20);
        }
        sl(destination,regs[0]);
        // MOVE.L preserves X, sets N/Z and clears V/C; no registers change.
        sw(frame,(w(frame)&~15)|(regs[0]==0?4:(regs[0]&0x80000000UL?8:0)));
        sl(frame+2,pc+4);return (uint32_t)usp;
    }

#ifdef POP2_SCENE_PROFILE
    if(g_sceneFinished)return stop(frame,usp,g_sceneResult?"SCENE PROFILE / NONZERO RETURN":"SCENE PROFILE / COMPLETE",trap,seg,off);
    if(!g_sceneReturn && g_integrationStage==3 && g_coreSegments[15][0]){
        for(uint32_t at=regs[14],depth=0;depth<32 && validMemory(at,10);++depth){
            uint32_t parent=l((uint8_t*)at),returned=l((uint8_t*)at+4);
            if(returned==g_coreSegments[2][0]+0x342 && w((uint8_t*)at+8)==POP2_SCENE_SELECTOR){
                if(w((uint8_t*)g_coreSegments[15][0]+4)!=0x4e56 ||
                   w((uint8_t*)g_coreSegments[15][0]+0x166)!=0x4e75)
                    return stop(frame,usp,"SCENE PROFILE / NIS OWNER",trap,seg,off);
                g_sceneReturn=returned;sl((uint8_t*)at+4,(uint32_t)pop2_scene_return);
                g_sceneActive=1;pop2SceneBoundary();break;
            }
            if(parent<=at)break;at=parent;
        }
    }
#endif
#ifdef POP2_GAMEPLAY_ROUTE_PROBE
    if(g_routeError)return stop(frame,usp,g_routeError,trap,seg,off);
#ifdef POP2_GAMEPLAY_HALL_PROBE
    if(g_routeHallComplete)return stop(frame,usp,"GAMEPLAY ROUTE / HALL COMPLETE",trap,seg,off);
    if(g_routeComplete && !g_routeHallReturn){
        for(uint32_t at=regs[14],depth=0;depth<32 && validMemory(at,10);++depth){
            uint32_t parent=l((uint8_t*)at),returned=l((uint8_t*)at+4);
            if(returned==g_coreSegments[2][0]+0x45c2){
                if(w((uint8_t*)at+8)!=w(a5-0x4fce) || w((uint8_t*)returned)!=0x544f)
                    return stop(frame,usp,"GAMEPLAY HALL / OWNER",trap,seg,off);
                g_routeHallReturn=returned;sl((uint8_t*)at+4,(uint32_t)pop2_gameplay_route_hall);break;
            }
            if(parent<=at)break;at=parent;
        }
    }
#else
    if(g_routeComplete)return stop(frame,usp,routeCompleteReason,trap,seg,off);
#endif
    if(routeEndingOrdinal>=0 && !g_routeEndingReturn && g_coreSegments[2][0]){
        // Observe the winning or time-expired story return without changing
        // either original session decision.
        const uint32_t terminalOffset=routeExpired ? 0x71a6 : 0x45b8;
        for(uint32_t at=regs[14],depth=0;depth<32 && validMemory(at,10);++depth){
            uint32_t parent=l((uint8_t*)at),returned=l((uint8_t*)at+4);
            if(returned==g_coreSegments[2][0]+terminalOffset){
                bool owner=routeExpired ?
                    w((uint8_t*)at+8)==28 && w(a5-0x4fce)==0 &&
                    l((uint8_t*)g_coreSegments[2][0]+0x719e)==0x701c3f00 &&
                    l((uint8_t*)g_coreSegments[2][0]+0x71a2)==0x4eba90f4 :
                    w((uint8_t*)at+8)==11 && w(a5-0x4f92)==15 &&
                    l((uint8_t*)g_coreSegments[2][0]+0x45a4)==0x0c44000e;
                if(!owner){
                    return stop(frame,usp,"GAMEPLAY ENDING / OWNER",trap,seg,off);
                }
                g_routeEndingReturn=returned;sl((uint8_t*)at+4,(uint32_t)pop2_gameplay_route_ending);break;
            }
            if(parent<=at)break;at=parent;
        }
    }
#endif
#ifdef POP2_INTEGRATION_PROBE
#ifdef POP2_CUTSCENE_ATTRACT_PROBE
    if(g_integrationStage==3)return stop(frame,usp,"GAMEPLAY / ATTRACT COMPLETE",trap,seg,off);
#endif
#ifdef POP2_GAMEPLAY_RESTART_PROBE
#ifdef POP2_GAMEPLAY_SAVE_PROBE
#ifdef POP2_GAMEPLAY_IMPORT_PROBE
    if(g_integrationStage==4)return stop(frame,usp,"GAMEPLAY / MAC SAVE IMPORT COMPLETE",trap,seg,off);
#else
    if(g_integrationStage==4)return stop(frame,usp,"GAMEPLAY / SAVE LOAD COMPLETE",trap,seg,off);
#endif
#else
#ifdef POP2_GAMEPLAY_HALL_VIEW_PROBE
    if(g_integrationStage==4)return stop(frame,usp,"GAMEPLAY / HALL VIEW COMPLETE",trap,seg,off);
#else
    if(g_integrationStage==4)return stop(frame,usp,"GAMEPLAY / RESTART COMPLETE",trap,seg,off);
#endif
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
#if !defined(POP2_GAMEPLAY_RESTART_PROBE) && !defined(POP2_GAMEPLAY_ROUTE_PROBE)
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
#if defined(POP2_VERIFY) && !defined(POP2_ROUTE_NO_TRAP_TRACE)
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
#if defined(POP2_GAMEPLAY_ROUTE_PROBE) && defined(POP2_ROUTE_AUDIO_TRACE)
        uint32_t selector=l(usp+4);
        if(g_integrationStage==3 && (selector==9 || selector==12 || selector==15 || selector==17 || selector==18 || selector==20)){
            if(g_routeAudioCount==8192)return stop(frame,usp,"GAMEPLAY ROUTE / AUDIO TRACE CAPACITY",trap,seg,off);
            auto& state=NativeAudio::state();auto* row=g_routeAudio[g_routeAudioCount++];
            row[0]=w(a5-0x349a);row[1]=selector;row[2]=state.clock;
            row[3]=selector==15 ? 0 : w((uint8_t*)l(usp+8)+(selector==9 || selector==12 ? 0 : 24));
            row[4]=regs[0];row[5]=state.cursors[5].position;
            row[6]=state.voices.effect.duration>=0;row[7]=wallTicks();
            row[8]=callerSeg;row[9]=callerOff;
        }
#endif
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
        uint16_t expected=seg==17 && off==0x245e?1:seg==17 && off==0x6d42?2:seg==2 && off==0x2220?3:seg==2 && off==0x20f4?4:0;
        if(!codeCaller || !expected)return stop(frame,usp,"UI / REPLACEMENT CALLER",trap,seg,off);
        uint16_t selector=w((uint8_t*)pc+2);
        if(selector!=expected)return stop(frame,usp,"UI / REPLACEMENT SELECTOR",trap,seg,off);
        if(!NativeUI::replacement(selector,regs,usp+4))return stop(frame,usp,NativeUI::error(),trap,seg,off);
        sl(frame+2,pc+4);return (uint32_t)usp;
    }
#endif
    uint32_t consumed=0;int16_t error=0;bool os=(trap&0x0800)==0;
    // As in Alone in the Dark, select a service by trap index rather than
    // searching every manager and a sparse compare tree on each call.
    // The table contains only port-owned labels; caller and patch checks above
    // remain common to every entry. Selector-dependent file traps retain their
    // explicit handles check inside that entry.
    static void* entries[4096];
    if(!entries[0]) {
        for(unsigned i=0;i<4096;++i)entries[i]=&&trap_unknown;
        entries[0xa84d&0xfff]=&&trap_0xa84d;
        entries[0xa868&0xfff]=&&trap_0xa868;
        entries[0xa800&0xfff]=&&trap_0xa800;
        entries[0xa51e&0xfff]=&&trap_0xa51e;
        entries[0xa033&0xfff]=&&trap_0xa033;
        entries[0xa034&0xfff]=&&trap_0xa034;
        entries[0xa9b4&0xfff]=&&trap_0xa9b4;
        entries[0xa05c&0xfff]=&&trap_0xa05c;
        entries[0xa05d&0xfff]=&&trap_0xa05d;
        entries[0xa85d&0xfff]=&&trap_0xa85d;
        entries[0xa9c6&0xfff]=&&trap_0xa9c6;
        entries[0xa058&0xfff]=&&trap_0xa058;
        entries[0xa059&0xfff]=&&trap_0xa059;
        entries[0xa05a&0xfff]=&&trap_0xa05a;
        entries[0xaa52&0xfff]=&&trap_0xaa52;
        entries[0xa997&0xfff]=&&trap_0xa997;
        entries[0xa994&0xfff]=&&trap_0xa994;
        entries[0xa9af&0xfff]=&&trap_0xa9af;
        entries[0xa998&0xfff]=&&trap_0xa998;
        entries[0xa99a&0xfff]=&&trap_0xa99a;
        entries[0xa9ab&0xfff]=&&trap_0xa9ab;
        entries[0xa9b0&0xfff]=&&trap_0xa9b0;
        entries[0xa9ad&0xfff]=&&trap_0xa9ad;
        entries[0xa9a4&0xfff]=&&trap_0xa9a4;
        entries[0xa9a8&0xfff]=&&trap_0xa9a8;
        entries[0xa9a0&0xfff]=&&trap_0xa9a0;
        entries[0xa9a5&0xfff]=&&trap_0xa9a5;
        entries[0xa9a6&0xfff]=&&trap_0xa9a6;
        entries[0xa9a2&0xfff]=&&trap_0xa9a2;
        entries[0xa992&0xfff]=&&trap_0xa992;
        entries[0xa9a3&0xfff]=&&trap_0xa9a3;
        entries[0xa02e&0xfff]=&&trap_0xa02e;
        entries[0xa31e&0xfff]=&&trap_0xa31e;
        entries[0xa021&0xfff]=&&trap_0xa021;
        entries[0xa01f&0xfff]=&&trap_0xa01f;
        entries[0xa023&0xfff]=&&trap_0xa023;
        entries[0xa049&0xfff]=&&trap_0xa049;
        entries[0xa04a&0xfff]=&&trap_0xa04a;
        entries[0xa128&0xfff]=&&trap_0xa128;
        entries[0xa11a&0xfff]=&&trap_0xa11a;
        entries[0xa01b&0xfff]=&&trap_0xa01b;
        entries[0xa055&0xfff]=&&trap_0xa055;
        entries[0xa01c&0xfff]=&&trap_0xa01c;
        entries[0xa11d&0xfff]=&&trap_0xa11d;
        entries[0xa162&0xfff]=&&trap_0xa162;
        entries[0xa036&0xfff]=&&trap_0xa036;
        entries[0xa069&0xfff]=&&trap_0xa069;
        entries[0xa06a&0xfff]=&&trap_0xa06a;
        entries[0xa02a&0xfff]=&&trap_0xa02a;
        entries[0xa024&0xfff]=&&trap_0xa024;
#ifdef POP2_INPUT
        entries[0xa975&0xfff]=&&trap_0xa975;
#endif
        entries[0xa040&0xfff]=&&trap_0xa040;
        entries[0xa9e1&0xfff]=&&trap_0xa9e1;
        entries[0xa025&0xfff]=&&trap_0xa025;
        entries[0xa029&0xfff]=&&trap_0xa029;
        entries[0xa064&0xfff]=&&trap_0xa064;
        entries[0xa122&0xfff]=&&trap_0xa122;
        entries[0xa11e&0xfff]=&&trap_0xa11e;
#ifdef POP2_UI
        entries[0xa746&0xfff]=&&trap_0xa746;
#endif
#ifdef POP2_UI
        entries[0xa346&0xfff]=&&trap_0xa346;
#endif
        entries[0xa146&0xfff]=&&trap_0xa146;
        entries[0xa047&0xfff]=&&trap_0xa047;
        entries[0xa9f0&0xfff]=&&trap_0xa9f0;
        entries[0xa9f4&0xfff]=&&trap_0xa9f4;
        entries[0xa1ad&0xfff]=&&trap_0xa1ad;
        entries[0xa090&0xfff]=&&trap_0xa090;
        entries[0xa8fe&0xfff]=&&trap_0xa8fe;
        entries[0xa86e&0xfff]=&&trap_0xa86e;
        for(unsigned i=0;i<4096;++i) {
            uint16_t t=0xa000|i;
            if(NativeFiles::handles(t,26))entries[i]=&&trap_files;
#ifdef POP2_DISPLAY
            if(NativeDisplay::handles(t))entries[i]=&&trap_display;
#ifdef POP2_UI
            if(NativeUI::handles(t))entries[i]=&&trap_ui;
#endif
#endif
        }
    }
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
#ifdef POP2_GAMEPLAY_HALL_VIEW_PROBE
        bool hallFrame=g_hallViewPhase==3 && displayBatchReturn==g_coreSegments[2][0]+0x616e;
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
#ifdef POP2_GAMEPLAY_HALL_VIEW_PROBE
        if(hallFrame){NativeDisplay::wait();g_hallViewPhase=4;g_integrationStage=4;recordIntegrationStage();}
#endif
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
#ifdef POP2_OPENING_PROFILE
        if(g_integrationStage==3 && g_openingCount<1024){
            volatile uint32_t* row=g_openingRows[g_openingCount];
            // Phases end when the Kid first lands on the roof (y 226, A5-50CC)
            // and at each health change.
            static uint16_t health;static bool landed;
            bool land=!landed && w(a5-0x50cc)==226;
            if(g_openingCount && (w(a5-0x50b4)!=health || land)){++g_openingPhases;pop2OpeningPhase();}
            health=w(a5-0x50b4);landed=landed||land;
            row[0]=w(a5-0x349a);row[1]=wallTicks();row[2]=g_scaffoldFields;row[3]=g_openingPhases;
            for(unsigned i=0;i<16;++i)row[4+i]=l(a5-0x50d2+4*i);
            ++g_openingCount;
        }
#endif
    }
#ifdef POP2_UI
    if(NativeInput::error())return stop(frame,usp,NativeInput::error(),trap,seg,off);
#endif
    if((trap&0xf000)!=0xa000)goto trap_unknown;
    goto *entries[trap&0xfff];
#ifdef POP2_UI
    { trap_ui:
#ifdef POP2_GAMEPLAY_HALL_VIEW_PROBE
        // Diagnostic input follows original wait callers, without debugger
        // writes to cached runtime state or changes to original control flow.
        if(trap==0xa860 && g_hallViewPhase && g_hallViewPhase<3){
            uint32_t code=g_coreSegments[2][0];
            if(g_hallViewPhase==1 && (uint32_t)usp<=g_hallViewOwner &&
               g_hallViewOwner-(uint32_t)usp<=2048 && validMemory(g_hallViewOwner,8) &&
               l((uint8_t*)g_hallViewOwner+4)==g_hallViewOwnerReturn){
                if(l((uint8_t*)code+0x2928)!=0x4ead1192)return stop(frame,usp,"HALL VIEW / WAIT CALL BYTES",trap,seg,off);
                for(uint32_t at=(uint32_t)usp;at+4<=g_hallViewOwner;at+=2){
                    if(l((uint8_t*)at)==code+0x292c){g_hallViewKeyRequest=0x45;g_hallViewPhase=2;break;}
                }
            }else if(g_hallViewPhase==2 && validMemory(regs[14],8) && l((uint8_t*)regs[14]+4)==code+0x5916){
                if(l((uint8_t*)code+0x5912)!=0x4ebad6f4)return stop(frame,usp,"HALL VIEW / PAUSE CALL BYTES",trap,seg,off);
                g_hallViewKeyRequest=0x40;g_hallViewPhase=3;
            }
        }
#endif
#ifdef POP2_GAMEPLAY_ROUTE_PROBE
        bool routeRead=trap==0xa976 && l(frame+2)==g_coreSegments[2][0]+0x5622 && g_integrationStage==3;
        if(routeRead && !routeKeyboard(w(a5-0x349a)))return stop(frame,usp,g_coreReason,trap,seg,off);
#endif
#ifdef POP2_GAMEPLAY_SAVE_PROBE
        bool gameplayRead=trap==0xa976 && l(frame+2)==g_coreSegments[2][0]+0x5622 && g_integrationStage>=3;
        uint16_t gameplayStep=w(a5-0x349a);
        if(gameplayRead)gameplayKeyboard(gameplayStep);
#endif
        if(!NativeUI::dispatch(trap,regs,usp,consumed))return stop(frame,usp,NativeUI::error(),trap,seg,off);
#ifdef POP2_GAMEPLAY_ROUTE_PROBE
        if(routeRead){
            uint8_t* keys=(uint8_t*)l(usp);
            for(unsigned i=0;i<16;++i)if(keys[i]!=routeKeys[g_routeKeys].keys[i])return stop(frame,usp,"GAMEPLAY ROUTE / KEY MAP",trap,seg,off);
            ++g_routeKeys;
        }
#endif
#ifdef POP2_GAMEPLAY_RESTART_PROBE
        if(trap==0xa93e && l(usp+2))g_gameplayMenu=l(usp+2);
#ifdef POP2_GAMEPLAY_SAVE_PROBE
        if(gameplayRead){
            if(g_gameplayKeyCount>=128)return stop(frame,usp,"GAMEPLAY / KEY TRACE CAPACITY",trap,seg,off);
            uint8_t* keys=(uint8_t*)l(usp);volatile uint32_t* row=g_gameplayKeys[g_gameplayKeyCount++];
            row[0]=gameplayStep;row[1]=keys[15]&8;row[2]=keys[15]&16;row[3]=keys[6]&128;
            if(gameplayStep==12 && (g_gameplayInputActions&1))gameplayEdges(0x21,false,0x66,false);
#ifdef POP2_GAMEPLAY_IMPORT_PROBE
            // Fresh Mac import consumes Command at step 23, then handles Open
            // before another gameplay keyboard read. Preserve that event order.
            if(gameplayStep==23 && !(g_gameplayInputActions&4)){
                gameplayEdges(0x18,true);gameplayEdges(0x18,false,0x66,false);g_gameplayInputActions|=4;
            }
#endif
        }
#endif
#endif
        if(os){uint16_t sr=w(frame)&~15;uint16_t result=regs[0];if(!result)sr|=4;if(result&0x8000)sr|=8;sw(frame,sr);}
        return (uint32_t)(usp+consumed);
    }
#endif
    { trap_display:
#if defined(POP2_UI_PROBE) || defined(POP2_STARTUP_CHECKPOINT)
    // Preserve the bounded earlier service fixtures after adding Palette2CTab.
    if(trap==0xaaa0 && seg==17 && off==0x820)
        return stop(frame,usp,"UI / STARTUP CHECKPOINT",trap,seg,off);
#endif
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
#ifdef POP2_GAMEPLAY_ROUTE_PROBE
                    g_routeReturn=ret;ret=(uint32_t)pop2_gameplay_route_frame;sl((uint8_t*)at+4,ret);
#endif
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
        if(trap==0xa8aa || trap==0xa8ab || trap==0xa8ae) {
            int32_t cc=trap==0xa8aa ? pureRect<0xa8aa,false>(usp,regs) :
                trap==0xa8ab ? pureRect<0xa8ab,false>(usp,regs) : pureRect<0xa8ae,false>(usp,regs);
            if(cc<0)return stop(frame,usp,"DISPLAY / PURE RECTANGLE EXTENT",trap,seg,off);
            consumed=trap==0xa8ae?4:12;sw(frame,(w(frame)&~31)|cc);
        } else if(!NativeDisplay::dispatch(trap,regs,usp,consumed))return stop(frame,usp,NativeDisplay::error(),trap,seg,off);
#if defined(POP2_GAMEPLAY_ROUTE_PROBE) && defined(POP2_ROUTE_RANDOM_TRACE)
        if(trap==0xa861 && g_integrationStage==3){
            uint16_t step=w(a5-0x349a);
            if((step>=523 && step<=551) || (step>=819 && step<=865) || (step>=1490 && step<=1545)){
                g_routeRandom[0]=step;g_routeRandom[1]=l(a5-0x18fc);
                g_routeRandom[2]=w(usp);g_routeRandom[3]=off+2;
                g_routeRandom[4]=l((uint8_t*)regs[14]+4);pop2GameplayRouteRandom();
            }
        }
#endif
        if(!displayBatchFrame && !NativeDisplay::publish())return stop(frame,usp,NativeDisplay::error(),trap,seg,off);
        return (uint32_t)(usp+consumed);
    }
#endif
#ifndef POP2_DISPLAY
    if((trap&0xf000)!=0xa000)goto trap_unknown;
    goto *entries[trap&0xfff];
#endif
    { trap_files:
        if(!NativeFiles::handles(trap,(uint16_t)regs[0]))goto trap_unknown;
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
        #ifdef POP2_GAMEPLAY_HALL_PROBE
        if(g_routeComplete && g_routeHallFileCount<16 && name){
            unsigned n=0;do{g_routeHallNames[g_routeHallFileCount][n]=name[n];}while(name[n++] && n<256);
        }
#endif
        int32_t fileError=NativeFiles::dispatch(trap,(uint16_t)regs[0],pb,buffer,name,nameOutput);
#ifdef POP2_GAMEPLAY_HALL_PROBE
        if(g_routeComplete && g_routeHallFileCount<16){
            volatile uint32_t* row=g_routeHallFiles[g_routeHallFileCount++];
            row[0]=trap;row[1]=fileError;row[2]=w(pb+22);row[3]=w(pb+24);row[4]=l(pb+36);row[5]=l(pb+40);
        }
#endif
        if(fileError==MacFiles::unsupported)return stop(frame,usp,"FILES / UNSUPPORTED OPERATION",trap,seg,off);
        sw(pb+16,fileError);regs[0]=fileError;
        uint16_t sr=w(frame)&~15;if(!(fileError&0xffff))sr|=4;if(fileError&0x8000)sr|=8;sw(frame,sr);
        return (uint32_t)usp;
    }
    switch(trap) {
    case 0xa84d: trap_0xa84d:case 0xa868: trap_0xa868:
        if(!fixedService(trap,usp))goto failed;
        consumed=8;break;
    case 0xa800: trap_0xa800: {
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
    case 0xa51e: trap_0xa51e:regs[8]=(uint32_t)heap.newPtr(regs[0],true);error=heap.error();regs[0]=(int32_t)error;break;
    case 0xa033: trap_0xa033:case 0xa034: trap_0xa034: {
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
    case 0xa9b4: trap_0xa9b4: // SystemTask: callbacks are delivered by the common service return.
        if(!trivialService<0xa9b4>(regs))goto failed;break;
    case 0xa05c: trap_0xa05c:
        if((regs[0]!=2 && regs[0]!=3) || !validMemory(regs[8],regs[9])){fail("MEMORY DISPATCH / LOCK RANGE");goto failed;}
        regs[0]=0;break; // LockMemory / UnlockMemory: native allocations are resident and physically stable.
    case 0xa05d: trap_0xa05d:
        if(!trivialService<0xa05d>(regs))goto failed;break;
    case 0xa85d: trap_0xa85d: { // BitTst numbers bits from the most significant bit.
        uint32_t bit=l(usp),address=l(usp+4)+(int32_t(bit)>>3);
        if(!validMemory(address,1)){fail("BITTEST / BYTE EXTENT");goto failed;}
        usp[8]=(*(uint8_t*)address&(0x80>>(bit&7)))?1:0;
        consumed=8;regs[0]=uint16_t(~bit);regs[1]=bit>>3;break;
    }
    case 0xa9c6: trap_0xa9c6: { // Secs2Date uses D0 seconds since 1904 and A0 DateTimeRec.
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
    case 0xa058: trap_0xa058:case 0xa059: trap_0xa059:case 0xa05a: trap_0xa05a:
        if(!timerService(trap,regs))goto failed;break;
    case 0xaa52: trap_0xaa52: {
        if((uint16_t)regs[0]!=1 || !validMemory((uint32_t)usp,16)) {fail("FILES / FSSPEC SELECTOR OR ARGUMENTS");goto failed;}
        char path[256];if(!pascalPath(l(usp+4),path) || !validMemory(l(usp),70)) {fail("FILES / FSSPEC EXTENT");goto failed;}
        int32_t result=NativeFiles::makeSpec((int16_t)w(usp+12),l(usp+8),path,(uint8_t*)l(usp));
        if(result==MacFiles::unsupported) {fail("FILES / FSSPEC UNSUPPORTED");goto failed;}
        sw(usp+14,result);regs[0]=result;consumed=14;break;
    }
    case 0xa997: trap_0xa997: {
        char path[256];if(!pascalPath(l(usp),path)) {fail("RESOURCE / FILE NAME EXTENT");goto failed;}
        int16_t ref=-1;resourceError=NativeResources::open(0,0,path,0,ref);
        if(resourceError==MacFiles::unsupported) {fail("RESOURCE / OPEN UNSUPPORTED");goto failed;}
        sw(usp+4,ref);sw(low+0xa60,resourceError);sw(low+0xa5a,NativeResources::current());consumed=4;break;
    }
    case 0xa994: trap_0xa994:sw(usp,NativeResources::current());break;
    case 0xa9af: trap_0xa9af:sw(usp,resourceError);break;
    case 0xa998: trap_0xa998:case 0xa99a: trap_0xa99a: {
        int16_t ref=(int16_t)w(usp);
        resourceError=trap==0xa998 ? NativeResources::use(ref) : publishResources(ref);
        if(!resourceError && trap==0xa99a)resourceError=NativeResources::close(ref);
        if(resourceError==MacFiles::unsupported) {fail("RESOURCE / FILE OPERATION UNSUPPORTED");goto failed;}
        if(!resourceError && trap==0xa99a)for(unsigned i=0;i<resourceCount;++i) {
            Resource& r=resources[i];if(r.handle && !r.detached && r.fileRef==ref) {
                if(heap.disposeHandle(r.handle)) {fail("RESOURCE / CLOSE HANDLE");goto failed;}r.handle=0;
            }
        }
        sw(low+0xa60,resourceError);sw(low+0xa5a,NativeResources::current());if(trap==0xa99a)regs[0]=(uint16_t)resourceError;consumed=2;break;
    }
    case 0xa9ab: trap_0xa9ab: {
        // Adapt Alone in the Dark's argument/ownership checks; metadata and
        // payload ownership remain in the shared ResourceDirectory.
        if(!validMemory((uint32_t)usp,14)) {fail("RESOURCE / ADD ARGUMENTS");goto failed;}
        auto h=(MacHeap::Handle)l(usp+10);uint32_t name=l(usp);
        Resource* existing=byHandle(h);
        if(!heap.isHandle(h) || !*h || (existing && !existing->detached) ||
           !validMemory(name,1) || !validMemory(name,uint32_t(*(uint8_t*)name)+1)) {
            fail("RESOURCE / ADD OWNERSHIP OR NAME");goto failed;
        }
        unsigned index=existing ? unsigned(existing-resources) : 0;
        if(!existing)while(index<resourceCount && resources[index].handle)++index;
        if(index==1024) {fail("RESOURCE / ADD SLOT CAPACITY");goto failed;}
        uint32_t size=heap.handleSize(h);const uint8_t* label=(const uint8_t*)name;
        ResourceWriter::Entry entry={l(usp+6),(int16_t)w(usp+4),0,label+1,label[0],{h,size,resourceHandleRead},0,size};
        NativeResources::Item item={};resourceError=NativeResources::add(entry,item);
        if(resourceError) {fail("RESOURCE / ADD UNMEASURED RESULT");goto failed;}
        Resource& r=resources[index];r.type=item.type;r.id=item.id;r.detached=false;
        r.identity=item.identity;r.fileRef=item.file;r.source={0,size,item.attributes};r.handle=h;r.changes=3;
        if(index==resourceCount)++resourceCount;
        error=heap.setState(h,(heap.state(h)|0x20)&~0x40);
        sw(low+0xa60,0);regs[0]=0;consumed=14;break;
    }
    case 0xa9b0: trap_0xa9b0: {
        auto h=(MacHeap::Handle)l(usp);Resource* r=byHandle(h);
        if(!r || r->detached || !heap.isHandle(h) || !*h) {fail("RESOURCE / WRITE OWNER");goto failed;}
        resourceError=(r->changes&1) ? publishResources(r->fileRef,r) : 0;
        if(resourceError) {fail("RESOURCE / WRITE PUBLICATION");goto failed;}
        error=0;sw(low+0xa60,0);regs[0]=0;consumed=4;break;
    }
    case 0xa9ad: trap_0xa9ad: {
        auto h=(MacHeap::Handle)l(usp);Resource* r=byHandle(h);
        if(!r || r->detached || !heap.isHandle(h) || !*h) {fail("RESOURCE / REMOVE OWNER");goto failed;}
        resourceError=NativeResources::remove(r->identity);
        if(resourceError) {fail("RESOURCE / REMOVE UNMEASURED RESULT");goto failed;}
        r->detached=true;r->changes=0;error=heap.setState(h,heap.state(h)&~0x20);
        sw(low+0xa60,0);regs[0]=0;consumed=4;break;
    }
    case 0xa9a4: trap_0xa9a4:case 0xa9a8: trap_0xa9a8: {
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
    case 0xa9a0: trap_0xa9a0: {
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
    case 0xa9a5: trap_0xa9a5: case 0xa9a6: trap_0xa9a6: case 0xa9a2: trap_0xa9a2: case 0xa992: trap_0xa992: case 0xa9a3: trap_0xa9a3: {
        auto h=(MacHeap::Handle)l(usp);Resource* r=byHandle(h);
        if(!r || !heap.isHandle(h)) { fail("RESOURCE / UNKNOWN HANDLE");goto failed; }
        if(trap==0xa9a5)sl(usp+4,r->source.size);
        if(trap==0xa9a6)sw(usp+4,r->source.attributes);
        if(trap==0xa9a2 && !load(*r))goto failed;
        if(trap==0xa992 && r->changes) {fail("RESOURCE / DETACH DIRTY HANDLE");goto failed;}
        if(trap==0xa992) { r->detached=true;heap.setState(h,heap.state(h)&~0x20);regs[0]=0; }
        if(trap==0xa9a3) {
#ifdef POP2_VERIFY
            if(seg==1 && off==0x120)copy(initial,world,g_coreBelow);
#endif
            if(r->detached) { fail("RESOURCE / RELEASE DETACHED");goto failed; }
            if(r->changes) {fail("RESOURCE / RELEASE DIRTY HANDLE");goto failed;}
            error=heap.disposeHandle(h);r->handle=0;r->detached=true;
        }
        consumed=4;break;
    }
    case 0xa02e: trap_0xa02e: {
        uint32_t n=regs[0],from=regs[8],to=regs[9];
        if(!validMemory(from,n)||!validMemory(to,n)){fail("MEMORY / BLOCKMOVE EXTENT");goto failed;}
        if(to>from && to-from<n)for(uint32_t i=n;i;--i)((uint8_t*)to)[i-1]=((const uint8_t*)from)[i-1];
        else copy((uint8_t*)to,(const uint8_t*)from,n);
        regs[0]=0;break;
    }
    case 0xa31e: trap_0xa31e:regs[8]=(uint32_t)heap.newPtr(regs[0],true);error=heap.error();regs[0]=(int32_t)error;break;
    case 0xa021: trap_0xa021:
        if(!validMemory(regs[8],1)){fail("HEAP / POINTER SIZE OWNER");goto failed;}
        regs[0]=heap.ptrSize((uint8_t*)regs[8]);
        if(heap.error()){
            auto handle=heap.recoverHandle((uint8_t*)regs[8]);
            if(!handle){fail("HEAP / POINTER SIZE BLOCK");goto failed;}
            regs[0]=heap.handleSize(handle);
        }
        break;
    case 0xa01f: trap_0xa01f:error=heap.disposePtr((uint8_t*)regs[8]);regs[0]=(int32_t)error;break;
    case 0xa023: trap_0xa023: {
        auto h=(MacHeap::Handle)regs[8];error=heap.disposeHandle(h);regs[0]=(int32_t)error;
        if(!error){Resource* r=byHandle(h);if(r)r->handle=nullptr;}break;
    }
    case 0xa049: trap_0xa049:case 0xa04a: trap_0xa04a: {
        auto h=(MacHeap::Handle)regs[8];
        if(FreedHandleCall::supported(trap,seg,off) && heap.isFreeHandleSlot(h)) {
            // Matched cleanup contracts preserve the free slot and return an error.
            // UnloadLevel can return -113 on the Mac; D0 is discarded and
            // MemError cleared before a read (docs/freed-handles.md).
            regs[0]=int32_t(MacHeap::memWZErr);sw(low+0x220,MacHeap::memWZErr);
            sw(frame,(w(frame)&~15)|8);return (uint32_t)usp;
        }
        if(!heap.isHandle(h)){fail("HEAP / PURGE HANDLE OWNER");goto failed;}
        error=heap.setState(h,trap==0xa049?heap.state(h)|0x40:heap.state(h)&~0x40);regs[0]=(int32_t)error;break;
    }
    case 0xa128: trap_0xa128:regs[8]=(uint32_t)heap.recoverHandle((uint8_t*)regs[8]);error=heap.error();regs[0]=(int32_t)error;break;
    case 0xa11a: trap_0xa11a:if(!trivialService<0xa11a>(regs))goto failed;break;
    case 0xa01b: trap_0xa01b:if(!trivialService<0xa01b>(regs))goto failed;break;
    case 0xa055: trap_0xa055:if(!trivialService<0xa055>(regs))goto failed;break;
    case 0xa01c: trap_0xa01c:regs[0]=heap.freeBytes();break;
    case 0xa11d: trap_0xa11d:regs[0]=heap.compact();regs[8]=0;break;
    case 0xa162: trap_0xa162:regs[0]=heap.freeBytes();regs[8]=heap.largestBlock();break;
    case 0xa036: trap_0xa036:error=heap.moreMasters();regs[0]=(int32_t)error;break;
    case 0xa069: trap_0xa069:case 0xa06a: trap_0xa06a: {
        auto h=(MacHeap::Handle)regs[8];if(!heap.isHandle(h)){fail("HEAP / STATE HANDLE OWNER");goto failed;}
        if(trap==0xa069){if(!trivialService<0xa069>(regs))goto failed;}
        else {error=heap.setState(h,uint8_t(regs[0]));regs[0]=(int32_t)error;}
        break;
    }
    case 0xa02a: trap_0xa02a:case 0xa024: trap_0xa024: {
#ifdef POP2_ROUTE_RESOURCE_TRACE
        if(seg==15 && off>=0x2700 && off<0x2a00){
            volatile uint32_t* row=g_routeHandles[(g_routeHandleCount++)%256];
            row[0]=trap;row[1]=seg;row[2]=off;row[3]=regs[8];
            row[4]=validMemory(regs[8],4)?l((uint8_t*)regs[8]):0xffffffff;
            row[5]=l(a5-0x2454);row[6]=l(a5-0x2450);row[7]=l(a5-0x244c);
        }
#endif
        auto h=(MacHeap::Handle)regs[8];
        if(FreedHandleCall::supported(trap,seg,off) && heap.isFreeHandleSlot(h)) {
            // MEASURED ending/scene-20 cleanup: disposed palette aliases return
            // memWZErr and preserves the free slot. Other unmeasured invalid
            // calls remain loud stops; freed-slot errors depend on Mac layout.
#ifdef POP2_ROUTE_RESOURCE_TRACE
            ++g_routeFreedUnlocks;
#endif
            regs[0]=int32_t(MacHeap::memWZErr);sw(low+0x220,MacHeap::memWZErr);
            sw(frame,(w(frame)&~15)|8);return (uint32_t)usp;
        }
        if(trap==0xa02a) {
            if(!trivialService<0xa02a>(regs)){fail("HEAP / UNKNOWN HANDLE");goto failed;}
            error=(int16_t)regs[0];
        } else {
            if(!heap.isHandle(h)){fail("HEAP / UNKNOWN HANDLE");goto failed;}
            error=heap.setHandleSize(h,regs[0]);regs[0]=(int32_t)error;
        }
        break;
    }
#ifdef POP2_INPUT
    case 0xa975: trap_0xa975: {
        uint32_t ticks=callbackTicks();sl(usp,ticks);regs[0]=0;
#if defined(POP2_SCENE_PROFILE) && defined(POP2_SCENE_DEADLINES)
        if(g_sceneActive && seg==16 && l(a5-0x2288+0x1c)!=(uint32_t)pop2_scene_animation && (off==0x128c || off==0x1358)) {
            uint8_t* slot=a5-0x2288+0x1c;uint32_t method=l(slot),methodSeg,methodOff;
            if(!validMemory(method,6) || w((uint8_t*)method)!=0x4ef9 ||
               !location(l((uint8_t*)method+2),methodSeg,methodOff) || methodSeg!=16 || methodOff!=0xb80)
                return stop(frame,usp,"SCENE DEADLINE / METHOD IDENTITY",trap,seg,off);
            g_sceneAnimationMethod=method;sl(slot,(uint32_t)pop2_scene_animation);
        }
#endif

#if defined(POP2_TIMING_PROBE) && defined(POP2_INTEGRATION_PROBE)
        if(g_integrationStage==3 && seg==17 && off==0x313e && g_timingSetCount<512){
            if(!validMemory(regs[14],14))return stop(frame,usp,"TIMING / SETTIMER FRAME",trap,seg,off);
            volatile uint32_t* row=g_timingSetRows[g_timingSetCount++];
            row[0]=w(a5-0x349a);row[1]=ticks;row[2]=w((uint8_t*)regs[14]+8);
            row[3]=w((uint8_t*)regs[14]+12);row[4]=g_scaffoldFields;
        }
#endif
#ifdef POP2_SCENE_PROFILE
        if(g_sceneActive && seg==15 && (off==0x3152 || off==0x321a || off==0x3984 || off==0x3b72)){
            if(g_sceneClockCount==2048 || regs[14]<0xb0 || !validMemory(regs[14]-0xb0,0xc4))
                return stop(frame,usp,"SCENE PROFILE / DISSOLVE FRAME OR CAPACITY",trap,seg,off);
            volatile uint32_t* row=g_sceneClocks[g_sceneClockCount++];uint8_t* owner=(uint8_t*)regs[14];
            row[0]=off;row[1]=ticks;row[2]=g_scaffoldFields;row[3]=l(owner+0x10);
            row[4]=off==0x321a?l(owner-0x6c):0;
            row[5]=off==0x321a?l(owner-0x7c)+row[4]:0;
            row[6]=off==0x321a?l(owner-0xa8):0;row[7]=g_routeFrames;
        }
        if(g_sceneActive && seg==17 && off==0x313e){
            if(g_sceneCount==4096 || !validMemory(regs[14],14))
                return stop(frame,usp,"SCENE PROFILE / TIMER CAPACITY OR FRAME",trap,seg,off);
            uint32_t callerSeg,callerOff;
            if(!location(l((uint8_t*)regs[14]+4),callerSeg,callerOff))
                return stop(frame,usp,"SCENE PROFILE / TIMER CALLER",trap,seg,off);
            volatile uint32_t* row=g_sceneRows[g_sceneCount++];
            row[0]=g_routeFrames;row[1]=ticks;row[2]=w((uint8_t*)regs[14]+8);
            row[3]=w((uint8_t*)regs[14]+12);row[4]=g_scaffoldFields;
            row[5]=callerSeg;row[6]=callerOff;row[7]=g_sceneActive;
        }
#endif
        break;
    }
#endif
    case 0xa040: trap_0xa040: // ResrvMem: reserve contiguous space in the current zone.
#ifdef POP2_GAMEPLAY_ROUTE_PROBE
        g_routeReserve[0]=regs[0];g_routeReserve[1]=heap.largestBlock();
#endif
        // Reserve requires the same compaction/purge path as allocation, not a
        // snapshot of the current free-block layout. Shared heap preserves
        // locked handles and non-purgeable data while making this space.
        error=heap.purge(regs[0]);regs[0]=(int32_t)error;
#ifdef POP2_GAMEPLAY_ROUTE_PROBE
        g_routeReserve[2]=heap.largestBlock();g_routeReserve[3]=uint16_t(error);
#endif
        break;
    case 0xa9e1: trap_0xa9e1: {
        auto source=(MacHeap::Handle)regs[8];
        if(!heap.isHandle(source) || !*source){fail("HEAP / HANDTOHAND SOURCE");goto failed;}
        MacHeap::Handle result=nullptr;error=HandleCopy::copy(heap,source,result);
        if(error){fail("HEAP / HANDTOHAND ALLOCATION");goto failed;}
        regs[8]=(uint32_t)result;regs[0]=0;
        sw(low+0x220,0);sw(frame,(w(frame)&~31)|4);break;
    }
    case 0xa025: trap_0xa025: {
        auto h=(MacHeap::Handle)regs[8];if(!heap.isHandle(h)) { fail("HEAP / UNKNOWN HANDLE");goto failed; }
        regs[0]=heap.handleSize(h);error=heap.error();break;
    }
    case 0xa029: trap_0xa029: case 0xa064: trap_0xa064: {
        auto h=(MacHeap::Handle)regs[8];if(!heap.isHandle(h)) { fail("HEAP / UNKNOWN HANDLE");goto failed; }
        if(trap==0xa029){if(!trivialService<0xa029>(regs))goto failed;}
        else {error=heap.moveHigh(h);regs[0]=(int32_t)error;}break;
    }
    case 0xa122: trap_0xa122:regs[8]=(uint32_t)heap.newHandle(regs[0],true);error=heap.error();regs[0]=(int32_t)error;break;
    case 0xa11e: trap_0xa11e: regs[8]=(uint32_t)heap.newPtr(regs[0],false);error=heap.error();regs[0]=(int32_t)error;break;
#ifdef POP2_UI
    case 0xa746: trap_0xa746:case 0xa346: trap_0xa346:{
        uint16_t target=trap==0xa746 ? 0xa800|(regs[0]&0x3ff) : 0xa000|(regs[0]&255);
        if(target==0xa0ad)target=0xa1ad;
        uint8_t* found=nullptr;
        for(uint8_t* p=(uint8_t*)pop2_cached_services;p<(uint8_t*)pop2_cached_services_end;p+=4)if(w(p+2)==target)found=p;
        if(!found)return stop(frame,usp,"TRAP TABLE / UNMEASURED QUERY",trap,seg,off);
        // Only these immutable pure builtins bypass the exception dispatcher.
        // ClipRect and the existing patched LoadSeg/Exit machinery stay there.
        if(target==0xa8aa)found=(uint8_t*)pop2SectRect;
        if(target==0xa8ab)found=(uint8_t*)pop2UnionRect;
        if(target==0xa8ae)found=(uint8_t*)pop2EmptyRect;
        regs[8]=(uint32_t)found;break;
    }
#endif
    case 0xa146: trap_0xa146: case 0xa047: trap_0xa047: {
        uint16_t target=regs[0];uint8_t** patch=target==0xa9f0?&patchedLoad:target==0xa9f4?&patchedExit:0;
        if(!patch) { fail("TRAP TABLE / UNKNOWN ENTRY");goto failed; }
        if(trap==0xa047) {
            bool exitEntry=seg==19 && off==0x033c && target==0xa9f4 && regs[8]==(uint32_t)a5+0x13fa;
            uint8_t* original=target==0xa9f0?(uint8_t*)pop2_old_loadseg:(uint8_t*)pop2_old_exit;
            // The runtime restores precisely the builtin address returned by
            // GetTrapAddress before freeing its two trampoline records.
            if(regs[8]==(uint32_t)original)*patch=nullptr;
            else {
                if(!heap.owns((void*)regs[8]) && !exitEntry) { fail("TRAP TABLE / UNKNOWN PATCH TARGET");goto failed; }
                *patch=(uint8_t*)regs[8];
            }
        }
        else regs[8]=(uint32_t)(*patch?*patch:target==0xa9f0?(uint8_t*)pop2_old_loadseg:(uint8_t*)pop2_old_exit);
        regs[0]=0;break;
    }
    case 0xa9f4: trap_0xa9f4: {
        // Alone in the Dark's ExitToShell policy: publish remaining resource
        // files before returning the host stack; immutable application stays open.
        for(;;) {
            int16_t ref=NativeResources::newest();
            if(ref<0){fail("RESOURCE / EXIT DIRECTORY");goto failed;}
            if(publishResources(ref)){fail("RESOURCE / EXIT PUBLICATION");goto failed;}
            if(ref==NativeResources::application())break;
            if(NativeResources::close(ref)){fail("RESOURCE / EXIT CLOSE");goto failed;}
        }
        uint32_t result=stop(frame,usp,"CORE / EXIT TO SHELL",trap,seg,off);
        timerCount=vblCount=0;normalExit=true;
        return result;
    }
    case 0xa9f0: trap_0xa9f0: {
        if(!builtin) { fail("LOADSEG / ORIGINAL HOOK MISSING");goto failed; }
        uint32_t resume=l(usp)-6;int16_t id=w(usp+4);
        uint32_t begin=(uint32_t)a5+app.world().jumpOffset;
        if(resume<begin+2 || resume>=begin+app.world().jumpBytes || ((resume-begin-2)&7)) {
            fail("LOADSEG / UNKNOWN RETURN TARGET");goto failed;
        }
        if(!segment(id))goto failed;
        usp+=6;sl(frame+2,resume);return (uint32_t)usp;
    }
    case 0xa1ad: trap_0xa1ad: {
        uint32_t answer;if(!MacIdentity::gestalt(regs[0],answer,error)) { fail("GESTALT / UNKNOWN SELECTOR");goto failed; }
        regs[0]=(int32_t)error;regs[8]=answer;error=0;break;
    }
    case 0xa090: trap_0xa090:
        if(!((regs[8]>=(uint32_t)stack && regs[8]-(uint32_t)stack<=65536-16) ||
             (regs[8]>=(uint32_t)world && regs[8]-(uint32_t)world<=worldSize-16) ||
             (regs[8]>=(uint32_t)arena && regs[8]-(uint32_t)arena<=g_coreZoneBytes-16) ||
             regs[8]==(uint32_t)g_coreSysEnv) || !MacIdentity::sysEnvirons(regs[0],(uint8_t*)regs[8])) {
            fail("SYSENV / UNSUPPORTED RECORD");goto failed;
        }
        regs[0]=0;break;
    case 0xa8fe: trap_0xa8fe:return stop(frame,usp,"UNIMPLEMENTED / InitFonts",trap,seg,off);
    case 0xa86e: trap_0xa86e:return stop(frame,usp,"UNIMPLEMENTED / InitGraf",trap,seg,off);
    default:trap_unknown:return stop(frame,usp,"UNIMPLEMENTED / TRAP",trap,seg,off);
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
    if(!g_timerCallbackActive || quickAudioPending())return 0;
    uint32_t pc=g_coreServicePC;
    if(!validMemory(pc,2))return 0;
    uint16_t trap=w((uint8_t*)pc);
    if(trap!=0xa058 && trap!=0xa059 && trap!=0xa05a && trap!=0xa868 && trap!=0xa84d && trap!=0xa975 && trap!=0xa11a && trap!=0xa01b)return 0;
    uint32_t seg=0,off=0;if(!location(pc,seg,off))return 0;
    uint8_t* usp=(uint8_t*)parked+66;
    if(usp<stack || usp>stack+65536-4)return 0;

#if defined(POP2_VERIFY) && !defined(POP2_ROUTE_NO_TRAP_TRACE)
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
    case 0xa11a:ok=trivialService<0xa11a>(parked);sw(low+0x220,heap.error());break;
    case 0xa01b:ok=trivialService<0xa01b>(parked);sw(low+0x220,heap.error());break;
    }
    g_coreServiceActive=0;
    if(!(trap&0x0800)) {
        // Timer calls preserve MemError, including a preceding HPurge error.
        // Zone calls publish the same heap status as the full dispatcher.
        uint16_t sr=w(frame)&~15,result=parked[0];
        if(!result)sr|=4;if(result&0x8000)sr|=8;sw(frame,sr);
    }
    if(!ok)stop(frame,usp,g_coreReason,trap,seg,off);
    uint8_t* image=(uint8_t*)parked+consumed;
    if(consumed)BlockMoveBytes::move((const uint8_t*)parked,image,60);
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
    uint32_t ticks=pop2ServiceTicks();
    if(!vblPass.needed(ticks))return 0;
    ticks=wallTicks();
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
    quickServicesValid=false;
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
        uint32_t field=g_scaffoldFields;
        bool candidate=false;
        for(unsigned i=0;i<timerCount;++i)
            if(timers[i].active && int32_t(field-timers[i].due.field)>=0){candidate=true;break;}
        TimerStamp now=candidate ? timerNow() : TimerStamp{field,0};
        // Installation/removal maintain the occupied extent, including holes.
        for(unsigned i=0;candidate && i<timerCount;++i){
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
    // The full path has just serviced the queues. Publish the same no-due
    // certificate for the following small trap instead of making it repeat
    // the cold checks. Interrupt epochs and all original owner guards still
    // expire or reject this certificate at the native entry.
    if(!didStop && !g_timerCallbackActive && quickServicesReady<false>()) {
        uint32_t seg,off;
        if(location(l(frame+2),seg,off)) {
            g_quickCodeBase=g_coreSegments[seg][0];
            g_quickCodeEnd=g_quickCodeBase+g_coreSegments[seg][1]-2;
        } else quickServicesValid=false;
    }
    uint8_t* image=resumed-66;
    if(image!=(uint8_t*)parked)BlockMoveBytes::move((const uint8_t*)parked,image,60);
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
