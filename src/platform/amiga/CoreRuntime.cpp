#ifdef POP2_CORE
#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/memory.h>
#include "CoreRuntime.h"
#include "framework/AmigaHardware.h"
#include "mac/MacHeap.h"
#include "mac/OriginalApplication.h"
#include "mac/SHA256.h"
#include "mac/Identity.h"
#include "../../../build/core/Inputs.h"
extern DosLibrary* DOSBase;
extern "C" {
void pop2_call_mac(void*,void*,void*);
void pop2_unknown_fixture();void pop2_illegal_fixture();
void pop2_identity_fixture();void pop2_identity_fixture_end();
uint32_t g_coreIdentity[7][3]={},g_coreSysEnvError=0;
uint8_t g_coreSysEnv[16]={};
void pop2_line_a();void pop2_mac_exit();void pop2_old_loadseg();void pop2_old_exit();
uint32_t g_coreException=0,g_coreActiveFrame=0,g_coreFaultPC=0;
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
uint32_t g_coreHostSP=0,g_coreTrace[128][3]={},g_coreCount=0,g_coreA5=0,g_coreBelow=0;
uint32_t g_coreSegments[24][2]={},g_coreInitial=0,g_coreStopTrap=0,g_coreStopSegment=0,g_coreStopOffset=0;
uint32_t g_coreZoneBytes=3*1024*1024,g_coreHeapOK=0,g_coreVectorRestored=0;
const char* g_coreReason="CORE / NOT STARTED";
__attribute__((noinline)) void pop2CoreStopped() { __asm__ volatile("nop" ::: "memory"); }
}
namespace {
uint16_t w(const uint8_t* p) { return uint16_t(p[0])<<8|p[1]; }
uint32_t l(const uint8_t* p) { return uint32_t(w(p))<<16|w(p+2); }
void sw(uint8_t* p,uint16_t v) { p[0]=v>>8;p[1]=v; }
void sl(uint8_t* p,uint32_t v) { sw(p,v>>16);sw(p+2,v); }
void copy(uint8_t* d,const uint8_t* s,uint32_t n) { while(n--)*d++=*s++; }
OriginalApplication app;MacHeap heap;
uint8_t *file=0,*plan=0,*arena=0,*world=0,*stack=0,*initial=0,*a5=0,*low=0;
uint32_t fileSize=0,planSize=0,worldSize=0;
bool didStop=false;
uint8_t auditBuffer[65536];
uint8_t* patchedLoad=0;uint8_t* patchedExit=0;
struct Resource { uint32_t type;int16_t id;OriginalApplication::Resource source;MacHeap::Handle handle;bool detached; };
Resource resources[32]={};unsigned resourceCount=0;
bool fail(const char* why) { g_coreReason=why;return false; }
bool readFile(const char* name,uint8_t*& data,uint32_t& size,uint32_t limit,const uint8_t digest[32]) {
    BPTR f=Open((CONST_STRPTR)name,MODE_OLDFILE);if(!f)return fail("CORE INPUT / OPEN");
    LONG seek=Seek(f,0,OFFSET_END);LONG n=seek<0 ? -1 : Seek(f,0,OFFSET_BEGINNING);
    if(n<=0 || uint32_t(n)>limit) { Close(f);return fail("CORE INPUT / SIZE"); }
    size=n;data=(uint8_t*)AllocMem(size,MEMF_PUBLIC);
    if(!data) { Close(f);return fail("CORE INPUT / MEMORY"); }
    LONG got=Read(f,data,size);Close(f);if(got!=(LONG)size)return fail("CORE INPUT / READ");
    SHA256 sha;uint8_t actual[32];sha_init(&sha);sha_update(&sha,data,size);sha_final(&sha,actual);
    for(unsigned i=0;i<32;++i)if(actual[i]!=digest[i])return fail("CORE INPUT / SHA256");
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
    CacheClearU();return true;
}
Resource* byHandle(MacHeap::Handle h) {
    for(unsigned i=0;i<resourceCount;++i)if(resources[i].handle==h)return resources+i;
    return 0;
}
bool load(Resource& r) {
    if(*r.handle)return true;
    if(heap.reallocateHandle(r.handle,r.source.size))return fail("RESOURCE / ALLOCATION");
    copy(*r.handle,r.source.bytes,r.source.size);
    if(r.type==0x53545253)g_coreStrs=(uint32_t)*r.handle;
#ifdef POP2_BAD_RELOCATION
    if(r.type==0x434f4445 && r.id==1)(*r.handle)[16]^=1;
#endif
    if(r.type==0x434f4445 && !relocate(r.id,*r.handle,r.source.size))return false;
    heap.setState(r.handle,0x20|((r.source.attributes&0x10)?0x80:0)|((r.source.attributes&0x20)?0x40:0));
    return true;
}
Resource* get(uint32_t type,int16_t id) {
    if(type!=0x434f4445 && type!=0x53545253 && type!=0x44415441 && type!=0x5a45524f &&
       type!=0x4452454c && type!=0x5f535444 && type!=0x5f535449) { fail("RESOURCE / UNIMPLEMENTED TYPE");return 0; }
    for(unsigned i=0;i<resourceCount;++i)if(resources[i].type==type && resources[i].id==id && !resources[i].detached) {
        if(low[0xa5e] && !load(resources[i]))return 0;
        return resources+i;
    }
    if(resourceCount==32) { fail("RESOURCE / SLOT CAPACITY");return 0; }
    Resource& r=resources[resourceCount];r.type=type;r.id=id;r.detached=false;
    if(!app.resource(type,id,r.source)) { fail("RESOURCE / NOT FOUND");return 0; }
    r.handle=heap.newEmptyHandle();if(!r.handle) { fail("RESOURCE / MASTER POINTER");return 0; }
    ++resourceCount;heap.setState(r.handle,0x20);
    if(low[0xa5e] && !load(r))return 0;
    return &r;
}
bool segment(int16_t id) {
    if(id<1 || id>=24)return fail("LOADSEG / SEGMENT ID");
    Resource* r=get(0x434f4445,id);if(!r || !load(*r))return false;
    uint8_t* code=*r->handle;uint32_t size=r->source.size;
    heap.setState(r->handle,heap.state(r->handle)|0x80);
    uint32_t first=w(code),count=w(code+2);auto spec=app.world();
    if(size<4 || (first&0x8007) || first>spec.jumpBytes || count>(spec.jumpBytes-first)/8)return fail("LOADSEG / NEAR HEADER");
    for(uint32_t i=0;i<count;++i) {
        uint8_t* entry=a5+spec.jumpOffset+first+8*i;
        if(w(entry+2)==0x4ef9 && w(entry)==id)continue;
        if(w(entry+2)!=0x3f3c || w(entry+4)!=id || w(entry+6)!=0xa9f0 || w(entry)>=size-4)return fail("LOADSEG / JUMP ENTRY");
        uint32_t target=(uint32_t)code+4+w(entry);
        sw(entry,id);sw(entry+2,0x4ef9);sl(entry+4,target);
    }
    g_coreSegments[id][0]=(uint32_t)code;g_coreSegments[id][1]=size;
    CacheClearU();return true;
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
bool location(uint32_t pc,uint32_t& seg,uint32_t& offset) {
    for(unsigned i=1;i<24;++i)if(pc>=g_coreSegments[i][0] && g_coreSegments[i][1]>=2 && pc-g_coreSegments[i][0]<=g_coreSegments[i][1]-2) {
        seg=i;offset=pc-g_coreSegments[i][0];return true;
    }
    return false;
}
uint32_t stop(uint8_t* frame,uint8_t* usp,const char* why,uint16_t trap,uint32_t seg,uint32_t off) {
    didStop=true;g_coreReason=why;g_coreStopTrap=trap;g_coreStopSegment=seg;g_coreStopOffset=off;
    sl(frame+2,(uint32_t)pop2_mac_exit);return (uint32_t)usp;
}
}
namespace CoreRuntime {
const char* error() { return g_coreReason; }
bool prepare() {
    if(!readFile("Prince of Persia 2.rsrc",file,fileSize,1024*1024,applicationSHA256))return false;
    if(!app.open(file,fileSize,applicationSHA256))return fail(app.error());
    if(!readFile("relocations.bin",plan,planSize,65536,relocationsSHA256))return false;
    if(planSize<4 || l(plan)>(65536-4)/32 || planSize!=4+32*l(plan))return fail("LOW MEMORY / PLAN SIZE");
    auto spec=app.world();if(spec.above>8192)return fail("A5 / SHADOW OVERLAP");
    worldSize=spec.below+8192+8192;
    arena=(uint8_t*)AllocMem(g_coreZoneBytes,MEMF_PUBLIC|MEMF_CLEAR);
    world=(uint8_t*)AllocMem(worldSize,MEMF_PUBLIC|MEMF_CLEAR);
    stack=(uint8_t*)AllocMem(65536,MEMF_PUBLIC|MEMF_CLEAR);
    initial=(uint8_t*)AllocMem(spec.below,MEMF_PUBLIC|MEMF_CLEAR);
    if(!arena || !world || !stack || !initial)return fail("CORE / MEMORY");
    if(!heap.init(arena,g_coreZoneBytes))return fail("CORE / APPLICATION ZONE");
    a5=world+spec.below;low=a5+8192;g_coreA5=(uint32_t)a5;g_coreBelow=spec.below;g_coreInitial=(uint32_t)initial;
    low[0xa5e]=0xff;sl(low+0x904,(uint32_t)a5);sl(low+0x908,(uint32_t)world);
    sw(low+0x28e,0x3fff);low[0x12f]=3;sw(low+0x15a,0x0755);
    OriginalApplication::Resource code0;app.resource(0x434f4445,0,code0);
    copy(a5+spec.jumpOffset,code0.bytes+16,spec.jumpBytes);
    if(!segment(1))return false;
    g_coreReason="CORE / RUNNING";return true;
}
void execute() {
    volatile uint32_t* vectors=(volatile uint32_t*)AmigaHardware::getVBR();
    const uint16_t indices[]={2,3,4,5,6,7,8,9,10,11,14};
    void (*handlers[])()={pop2_exception_2,pop2_exception_3,pop2_exception_4,pop2_exception_5,
        pop2_exception_6,pop2_exception_7,pop2_exception_8,pop2_exception_9,pop2_line_a,pop2_exception_11,pop2_exception_14};
    uint32_t saved[11];Disable();
    for(unsigned i=0;i<11;++i) { saved[i]=vectors[indices[i]];vectors[indices[i]]=(uint32_t)handlers[i]; }
    Enable();
#ifdef POP2_UNKNOWN_TRAP
    pop2_call_mac((void*)pop2_unknown_fixture,a5,stack+65536);
#elif defined(POP2_ILLEGAL)
    pop2_call_mac((void*)pop2_illegal_fixture,a5,stack+65536);
#else
    pop2_call_mac((void*)pop2_identity_fixture,a5,stack+65536);
#endif
    if(!didStop) {
        pop2_call_mac((void*)(g_coreSegments[1][0]+16),a5,stack+65536);
        if(!didStop)fail("CORE / ORIGINAL RETURN WITHOUT STOP");
    }
    Disable();g_coreVectorRestored=1;
    for(unsigned i=0;i<11;++i) { vectors[indices[i]]=saved[i];if(vectors[indices[i]]!=saved[i])g_coreVectorRestored=0; }
    Enable();
    g_coreHeapOK=heap.check();
    if(g_coreStopTrap==0xa86e && g_coreStopSegment==2 && g_coreStopOffset==0x3e90) {
#ifdef POP2_BAD_CODE_WRITE
        ((uint8_t*)g_coreSegments[2][0])[4]^=1;
#endif
        auditCode();
    }
}
void release() {
    app.close();heap.reset();
    if(initial)FreeMem(initial,g_coreBelow);if(stack)FreeMem(stack,65536);if(world)FreeMem(world,worldSize);
    if(arena)FreeMem(arena,g_coreZoneBytes);if(plan)FreeMem(plan,planSize);if(file)FreeMem(file,fileSize);
    initial=stack=world=arena=plan=file=0;
}
}
extern "C" uint32_t pop2Dispatch(uint32_t* regs,uint8_t* frame,uint8_t* usp) {
    uint32_t pc=l(frame+2),seg=0,off=0;
    if(usp<stack || usp>stack+65536-4)return stop(frame,usp,"STACK / TRAP ARGUMENT EXTENT",0,seg,off);
    bool codeCaller=location(pc,seg,off);
    bool tableCaller=pc>=(uint32_t)a5+app.world().jumpOffset && pc<(uint32_t)a5+app.world().jumpOffset+app.world().jumpBytes;
    bool fixtureCaller=pc>=(uint32_t)pop2_identity_fixture && pc<(uint32_t)pop2_identity_fixture_end;
    if(!codeCaller && !tableCaller && !fixtureCaller && pc!=(uint32_t)pop2_old_loadseg && pc!=(uint32_t)pop2_old_exit)
        return stop(frame,usp,"TRAP / UNKNOWN CALLER",0,seg,off);
    uint16_t trap=w((uint8_t*)pc);
    if((trap&0x0800) && !fixtureCaller && usp>stack+65536-10)
        return stop(frame,usp,"STACK / TOOLBOX ARGUMENT EXTENT",trap,seg,off);
    bool builtin=pc==(uint32_t)pop2_old_loadseg || pc==(uint32_t)pop2_old_exit;
    if(builtin)trap=w((uint8_t*)pc+2);
    if(codeCaller) {
        if(g_coreCount==128)return stop(frame,usp,"TRACE / CAPACITY",trap,seg,off);
        uint32_t* row=g_coreTrace[g_coreCount++];row[0]=seg;row[1]=off;row[2]=trap;
    } else if(!builtin && !(pc>=(uint32_t)pop2_identity_fixture && pc<(uint32_t)pop2_identity_fixture_end) && !(pc>=(uint32_t)a5+app.world().jumpOffset && pc<(uint32_t)a5+app.world().jumpOffset+app.world().jumpBytes && trap==0xa9f0))
        return stop(frame,usp,"TRAP / UNKNOWN CALLER",trap,seg,off);
    sl(frame+2,pc+(builtin?4:2));
    if(!builtin && (trap==0xa9f0 || trap==0xa9f4)) {
        uint8_t* patch=trap==0xa9f0?patchedLoad:patchedExit;
        if(patch) { CacheClearU();usp-=4;sl(usp,pc+2);sl(frame+2,(uint32_t)patch);return (uint32_t)usp; }
    }
    uint32_t consumed=0;int16_t error=0;bool os=(trap&0x0800)==0;
    switch(trap) {
    case 0xa9a0: {
        Resource* r=get(l(usp+2),(int16_t)w(usp));if(!r)goto failed;
        sl(usp+6,(uint32_t)r->handle);regs[0]=0;
        if(g_coreResourceCount==32) { fail("RESOURCE / TRACE CAPACITY");goto failed; }
        { uint32_t* row=g_coreResources[g_coreResourceCount++];row[0]=r->type;row[1]=(uint16_t)r->id;row[2]=(uint32_t)r->handle; }
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
            if(seg==1 && off==0x120)copy(initial,world,g_coreBelow);
            if(r->detached) { fail("RESOURCE / RELEASE DETACHED");goto failed; }
            error=heap.disposeHandle(h);r->handle=0;r->detached=true;
        }
        consumed=4;break;
    }
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
    case 0xa11e: regs[8]=(uint32_t)heap.newPtr(regs[0],false);error=heap.error();regs[0]=(int32_t)error;break;
    case 0xa146: case 0xa047: {
        uint16_t target=regs[0];uint8_t** patch=target==0xa9f0?&patchedLoad:target==0xa9f4?&patchedExit:0;
        if(!patch) { fail("TRAP TABLE / UNKNOWN ENTRY");goto failed; }
        if(trap==0xa047) {
            if(!heap.owns((void*)regs[8])) { fail("TRAP TABLE / UNKNOWN PATCH TARGET");goto failed; }
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
    case 0xa86e:return stop(frame,usp,"UNIMPLEMENTED / InitGraf",trap,seg,off);
    default:return stop(frame,usp,"UNIMPLEMENTED / TRAP",trap,seg,off);
    }
    if(error) { fail("CORE / MEMORY SERVICE ERROR");goto failed; }
    if(os) {
        // Macintosh OS traps return TST.W D0 flags; Toolbox preserves CCR.
        uint16_t sr=w(frame)&~15;uint16_t result=regs[0];
        if(!result)sr|=4;if(result&0x8000)sr|=8;sw(frame,sr);
    }
    return (uint32_t)(usp+consumed);
failed:return stop(frame,usp,g_coreReason,trap,seg,off);
}
extern "C" uint32_t pop2Exception(uint8_t* frame) {
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
