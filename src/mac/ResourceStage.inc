// Shared DOS publication implementation extracted from Alone in the Dark.
#include <proto/dos.h>
#include <dos/dos.h>
#include "ResourceStage.h"
// The including adapter supplies FileAccess and RESOURCE_STAGE_SYSTEM_WINDOW.
extern "C" {
#include <string.h>
}
#ifndef RESOURCE_STAGE_SYSTEM_WINDOW
#error Define RESOURCE_STAGE_SYSTEM_WINDOW to the port system-window bridge
#endif
#if defined(AITD_FILE_WRITE_PROBE) || defined(AITD_RESOURCE_EXIT_PROBE)
extern "C" { volatile uint32_t g_resourceStageFault=0; }
static bool fault(uint32_t n) { return g_resourceStageFault==n; }
#else
static bool fault(uint32_t) { return false; }
#endif
static int32_t present(const char* path,bool& found,bool regular=false) {
    BPTR lock=Lock((CONST_STRPTR)path,ACCESS_READ);found=lock!=0;
    if(!lock)return IoErr()==ERROR_OBJECT_NOT_FOUND ? 0 : -36;
    int32_t error=0;
    if(regular) {
        FileInfoBlock* info=(FileInfoBlock*)AllocDosObject(DOS_FIB,0);
        error=!info ? -108 : !Examine(lock,info) ? -36 : info->fib_DirEntryType>=0 ? -50 : (info->fib_Protection&FIBF_WRITE) ? -45 : 0;
        if(info)FreeDosObject(DOS_FIB,info);
    }
    UnLock(lock);return error;
}
static bool suffix(char* out,const char* path,const char* ending) {
    volatile char* destination=out;uint16_t n=0;while(*path) { if(n>=191)return false;destination[n++]=*path++; }
    while(*ending) { if(n>=191)return false;destination[n++]=*ending++; }destination[n]=0;return true;
}
bool ResourceStage::bind(const char* path) {
    if(active() || !path || !*path)return false;
    char check[192];if(!suffix(check,path,".aitd-old"))return false;
    return suffix(path_,path,"") && suffix(temporary_,path,".aitd-new") && suffix(backup_,path,".aitd-old");
}
struct StageRequest { ResourceStage* stage;uint32_t size,offset,actual;const uint8_t* bytes;bool publish; };
int32_t ResourceStage::begin(void* context,uint32_t size) {
    if(!context || size>0x7fffffffUL)return -50;
    auto& s=*(ResourceStage*)context;if(!s.path_[0] || s.active())return recoveryRequired;
    if(FileAccess::resloadActive()) {
        if(size>2UL*1024*1024)return recoveryRequired;
        uint32_t previousSize=0;bool exists=false,locked=false;
        int32_t error=FileAccess::resloadStat(s.temporary_,previousSize,exists,locked);
        if(error || exists)return error ? error : recoveryRequired;
        error=FileAccess::resloadStat(s.backup_,previousSize,exists,locked);
        if(error || exists)return error ? error : recoveryRequired;
        error=FileAccess::resloadStat(s.path_,previousSize,s.previous_,locked);
        if(error || locked)return error ? error : -45;
        s.resloadBytes_=new uint8_t[size?size:1];if(!s.resloadBytes_)return -108;
        s.handle_=1;s.owned_=true;s.size_=size;s.written_=0;s.poisoned_=false;return 0;
    }
    StageRequest r={&s,size,0,0,0,false};return RESOURCE_STAGE_SYSTEM_WINDOW(beginInside,&r);
}
int32_t ResourceStage::beginInside(void* context) {
    auto& r=*(StageRequest*)context;auto& s=*r.stage;bool exists=false;
    int32_t error=present(s.temporary_,exists);if(error || exists)return error ? error : recoveryRequired;
    error=present(s.backup_,exists);if(error || exists)return error ? error : recoveryRequired;
    error=present(s.path_,s.previous_,true);if(error)return error;
    s.handle_=Open((CONST_STRPTR)s.temporary_,MODE_NEWFILE);if(!s.handle_)return -36;
    s.owned_=true;s.size_=r.size;s.written_=0;s.poisoned_=false;return 0;
}
int32_t ResourceStage::write(void* context,uint32_t offset,const uint8_t* bytes,uint32_t size,uint32_t& actual) {
    actual=0;if(!context || (!bytes && size) || size>65536)return -50;
    auto& s=*(ResourceStage*)context;
    if(!s.handle_ || !s.owned_ || s.failed_ || s.poisoned_ || offset!=s.written_ || offset>s.size_ || size>s.size_-offset)return -50;
    if(s.resloadBytes_) { if(size)memcpy(s.resloadBytes_+offset,bytes,size);s.written_+=size;actual=size;return 0; }
    StageRequest r={&s,size,offset,0,bytes,false};int32_t error=RESOURCE_STAGE_SYSTEM_WINDOW(writeInside,&r);actual=r.actual;return error;
}
int32_t ResourceStage::writeInside(void* context) {
    auto& r=*(StageRequest*)context;auto& s=*r.stage;
    LONG actual=Write(s.handle_,(APTR)r.bytes,r.size);r.actual=actual<0 ? 0 : actual;s.written_+=r.actual;
    if(r.actual!=r.size || actual<0) { s.poisoned_=true;return -36; }return 0;
}
int32_t ResourceStage::finish(void* context,bool publish) {
    if(!context)return -50;
    auto& s=*(ResourceStage*)context;
    if(s.resloadBytes_) {
        if(publish) {
            if(s.poisoned_ || s.written_!=s.size_)return -36;
            int32_t result=FileAccess::resloadReplace(s.path_,s.resloadBytes_,s.size_);if(result)return result;
        }
        delete[] s.resloadBytes_;s.resloadBytes_=0;s.handle_=0;s.owned_=false;s.poisoned_=false;return 0;
    }
    StageRequest r={(ResourceStage*)context,0,0,0,0,publish};return RESOURCE_STAGE_SYSTEM_WINDOW(finishInside,&r);
}
int32_t ResourceStage::finishInside(void* context) {
    auto& r=*(StageRequest*)context;auto& s=*r.stage;
    if(s.failed_)return recoveryRequired;
    if(!r.publish) {
        bool closed=true;if(s.handle_) { closed=Close(s.handle_);s.handle_=0; }
        if(!closed || (s.owned_ && !DeleteFile((CONST_STRPTR)s.temporary_))) { s.failed_=true;return recoveryRequired; }
        s.owned_=false;s.poisoned_=false;return 0;
    }
    if(!s.owned_ || !s.handle_ || s.poisoned_ || s.written_!=s.size_)return -36;
    LONG flushed=Flush(s.handle_),closed=Close(s.handle_);s.handle_=0;
    if(!flushed || !closed)return -36;
    if(s.previous_ && !Rename((CONST_STRPTR)s.path_,(CONST_STRPTR)s.backup_))return -36;
    if(fault(1) || !Rename((CONST_STRPTR)s.temporary_,(CONST_STRPTR)s.path_)) {
        if(s.previous_ && !Rename((CONST_STRPTR)s.backup_,(CONST_STRPTR)s.path_)) { s.failed_=true;return recoveryRequired; }
        return -36;
    }
    if(s.previous_ && (fault(2) || !DeleteFile((CONST_STRPTR)s.backup_))) {
        if(!Rename((CONST_STRPTR)s.path_,(CONST_STRPTR)s.temporary_) || !Rename((CONST_STRPTR)s.backup_,(CONST_STRPTR)s.path_)) { s.failed_=true;return recoveryRequired; }
        return -36;
    }
    s.owned_=false;s.poisoned_=false;return 0;
}
