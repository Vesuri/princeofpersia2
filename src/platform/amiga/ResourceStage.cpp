#ifdef POP2_CORE
#include <proto/dos.h>
#include <dos/dos.h>
#include "SystemWindow.h"
#include "mac/MacFiles.h"
// Owner decision: never rename. WHDLoad's kickfs has no ACTION_RENAME_OBJECT and
// resload has no rename. The shared transaction's whole-file mode buffers the new
// fork, which is serialized from the old one, then replaces the target in place
// as the original Resource Manager does. Both environments use DOS: MODE_NEWFILE
// truncates through kickfs, which has no ACTION_SET_FILE_SIZE either.
namespace FileAccess {
struct Stat { const char* path;uint32_t size;bool exists,locked; };
struct Replace { const char* path;const uint8_t* bytes;uint32_t size; };
static int32_t statInside(void* context) {
    Stat& s=*(Stat*)context;
    BPTR lock=Lock((CONST_STRPTR)s.path,ACCESS_READ);
    if(!lock)return IoErr()==ERROR_OBJECT_NOT_FOUND ? 0 : -36;
    FileInfoBlock* info=(FileInfoBlock*)AllocDosObject(DOS_FIB,0);
    int32_t error=!info ? -108 : !Examine(lock,info) ? -36 : info->fib_DirEntryType>=0 ? -50 : 0;
    if(!error) { s.exists=true;s.size=info->fib_Size;s.locked=(info->fib_Protection&FIBF_WRITE)!=0; }
    if(info)FreeDosObject(DOS_FIB,info);
    UnLock(lock);return error;
}
static int32_t replaceInside(void* context) {
    Replace& r=*(Replace*)context;
    BPTR f=Open((CONST_STRPTR)r.path,MODE_NEWFILE);if(!f)return -36;
    int32_t error=0;
    for(uint32_t at=0;!error && at<r.size;) {
        uint32_t n=r.size-at;if(n>65536)n=65536;
        if(Write(f,(APTR)(r.bytes+at),n)!=(LONG)n)error=-36;
        at+=n;
    }
    if(!Close(f))error=-36;
    return error;
}
static bool resloadActive() { return true; }
static int32_t resloadStat(const char* path,uint32_t& size,bool& exists,bool& locked) {
    Stat s={path,0,false,false};int32_t error=pop2SystemWindow(statInside,&s);
    size=s.size;exists=s.exists;locked=s.locked;return error;
}
static int32_t resloadReplace(const char* path,const uint8_t* bytes,uint32_t size) {
    Replace r={path,bytes,size};return pop2SystemWindow(replaceInside,&r);
}
}
#define RESOURCE_STAGE_SYSTEM_WINDOW pop2SystemWindow
#include "mac/ResourceStage.inc"
#endif
