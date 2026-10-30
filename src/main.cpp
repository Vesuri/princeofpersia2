#ifdef POP2_PLATFORM_AMIGA
#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dosextens.h>
#include <dos/var.h>
#include <workbench/startup.h>
#include "platform/amiga/PlatformAmiga.h"
static PlatformAmiga platform;
int main() {
    Process* process=(Process*)FindTask(0);
    WBStartup* message=0;
    if(!process->pr_CLI) { WaitPort(&process->pr_MsgPort);message=(WBStartup*)GetMsg(&process->pr_MsgPort); }
    int result=platform.run() ? 0 : 20;
    // The WHDLoad slave shows this reason in its failure requester.
    if(result && platform.reason()) {
        DOSBase=(DosLibrary*)OpenLibrary((CONST_STRPTR)"dos.library",39);
        if(DOSBase) { SetVar((CONST_STRPTR)"Pop2Reason",(CONST_STRPTR)platform.reason(),-1,GVF_LOCAL_ONLY);CloseLibrary((Library*)DOSBase);DOSBase=0; }
    }
    if(message) { Forbid();ReplyMsg(&message->sm_Message); }
    return result;
}
#else
#include <cstdio>
int main() { std::puts("Prince of Persia 2: empty host scaffold");return 0; }
#endif
