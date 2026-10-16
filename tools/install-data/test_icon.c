/* Native regression for the generated Workbench icon. Not part of the helper. */
#include <exec/execbase.h>
#include <workbench/workbench.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/icon.h>
struct ExecBase *SysBase;
struct DosLibrary *DOSBase;
struct Library *IconBase;
static int equal(const char *a,const char *b) {
    if(!a || !b) return 0;
    while(*a && *a==*b) { a++; b++; }
    return *a==*b;
}
int amiga_main(void) {
    struct DiskObject *icon; BPTR f; int ok=0;
    __asm__ volatile("move.l 4.w,%0":"=r"(SysBase));
    DOSBase=(struct DosLibrary *)OpenLibrary((CONST_STRPTR)"dos.library",37);
    if(DOSBase) {
        BPTR lock=CreateDir((CONST_STRPTR)"RAM:T"); if(lock) UnLock(lock);
        lock=CreateDir((CONST_STRPTR)"RAM:ENV"); if(lock) UnLock(lock);
        lock=Lock((CONST_STRPTR)"RAM:T",ACCESS_READ);
        if(lock && !AssignLock((CONST_STRPTR)"T",lock)) UnLock(lock);
        lock=Lock((CONST_STRPTR)"RAM:ENV",ACCESS_READ);
        if(lock && !AssignLock((CONST_STRPTR)"ENV",lock)) UnLock(lock);
    }
    IconBase=OpenLibrary((CONST_STRPTR)"icon.library",37);
    if(DOSBase && IconBase) {
        icon=GetDiskObject((CONST_STRPTR)"DH0:Install");
        if(icon) {
            ok=icon->do_Type==WBPROJECT && equal((const char *)icon->do_DefaultTool,"Installer")
                && icon->do_ToolTypes && equal((const char *)FindToolType(icon->do_ToolTypes,(CONST_STRPTR)"APPNAME"),"Prince of Persia 2")
                && icon->do_StackSize==4096 && icon->do_Gadget.GadgetRender
                && icon->do_Gadget.Width==70 && icon->do_Gadget.Height==26
                && equal((const char *)FindToolType(icon->do_ToolTypes,(CONST_STRPTR)"MINUSER"),"AVERAGE")
                && equal((const char *)FindToolType(icon->do_ToolTypes,(CONST_STRPTR)"PRETEND"),"FALSE")
                && !FindToolType(icon->do_ToolTypes,(CONST_STRPTR)"SCRIPT");
            FreeDiskObject(icon);
        }
        if(ok) {
            icon=GetDiskObject((CONST_STRPTR)"DH0:GameTemplate");
            ok=icon && icon->do_Type==WBPROJECT && icon->do_StackSize==4096
                && icon->do_Gadget.GadgetRender && !icon->do_DefaultTool;
            if(icon) FreeDiskObject(icon);
        }
        if(ok) {
            icon=GetDiskObject((CONST_STRPTR)"DH0:ReadMe");
            ok=icon && icon->do_Type==WBPROJECT && icon->do_Gadget.GadgetRender
                && equal((const char *)icon->do_DefaultTool,"MultiView");
            if(icon) FreeDiskObject(icon);
        }
        if(ok) {
            icon=GetDiskObject((CONST_STRPTR)"DH0:Package");
            ok=icon && icon->do_Type==WBDRAWER && icon->do_DrawerData
                && icon->do_Gadget.GadgetRender;
            if(icon) FreeDiskObject(icon);
        }
        if(ok && (f=Open((CONST_STRPTR)"DH2:icon-ok",MODE_NEWFILE))) { Write(f,(APTR)"OK\n",3); Close(f); }
        /* Called again after Installer: check its configured game icon too. */
        if(ok) {
            BPTR lock=Lock((CONST_STRPTR)"DH2:out/Prince of Persia 2/PrinceOfPersia2.info",ACCESS_READ);
            if(lock) {
                UnLock(lock);
                icon=GetDiskObject((CONST_STRPTR)"DH2:out/Prince of Persia 2/PrinceOfPersia2");
                ok=icon && icon->do_Type==WBPROJECT && icon->do_StackSize==10240
                    && equal((const char *)icon->do_DefaultTool,"WHDLoad")
                    && equal((const char *)FindToolType(icon->do_ToolTypes,(CONST_STRPTR)"SLAVE"),"PrinceOfPersia2.slave")
                    && FindToolType(icon->do_ToolTypes,(CONST_STRPTR)"PRELOAD");
                if(icon) FreeDiskObject(icon);
                if(ok && (f=Open((CONST_STRPTR)"DH2:installed-icon-ok",MODE_NEWFILE))) {
                    Write(f,(APTR)"OK\n",3); Close(f);
                }
            }
        }
    }
    if(IconBase) CloseLibrary(IconBase);
    if(DOSBase) CloseLibrary((struct Library *)DOSBase);
    return ok?0:20;
}
