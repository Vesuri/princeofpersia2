#include "install_config.h"
#include "io.h"
#include <exec/execbase.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <string.h>
struct ExecBase *SysBase;
struct DosLibrary *DOSBase;
void *io_alloc(uint32_t n) { return AllocVec(n,MEMF_PUBLIC); }
void io_free(void *p) { if(p) FreeVec(p); }
static const char version[] __attribute__((used,section(".text.ver"))) = INSTALL_VERSION;
#ifdef INSTALL_STACK_TEST
volatile uint32_t installer_stack_size,installer_stack_unused;
volatile int installer_result;
__attribute__((noinline)) void installer_test_done(void) { __asm__ volatile("nop"); }
static unsigned char *stack_lower;
static void stack_mark(struct Process *p) {
    unsigned char *sp; volatile unsigned char *mark;
    __asm__ volatile("move.l %%sp,%0" : "=a"(sp));
    stack_lower=p->pr_Task.tc_SPLower;
    installer_stack_size=(unsigned char *)p->pr_Task.tc_SPUpper-stack_lower;
    for(mark=stack_lower;mark<sp-128;mark++) *mark=0xa7;
}
static void stack_check(int result) {
    unsigned char *p=stack_lower;
    while((uint32_t)(p-stack_lower)<installer_stack_size && *p==0xa7) p++;
    installer_stack_unused=p-stack_lower; installer_result=result;
    installer_test_done();
}
#endif
int io_cancelled(void) { return (SetSignal(0,0)&SIGBREAKF_CTRL_C)!=0; }
int io_open(File *f,const char *p,int w) {
    BPTR h=Open((CONST_STRPTR)p,w?MODE_NEWFILE:MODE_OLDFILE); LONG size;
    if(!h) return 0;
    f->handle=(void *)h; f->pos=f->size=0;
    if(!w) {
        if(Seek(h,0,OFFSET_END)<0 || (size=Seek(h,0,OFFSET_BEGINNING))<0) { io_close(f); return 0; }
        f->size=(uint32_t)size;
    }
    return 1;
}
int io_read(File *f,void *b,uint32_t n) {
    if(Read((BPTR)f->handle,b,(LONG)n)!=(LONG)n) return 0;
    f->pos+=n; return 1;
}
int io_write(File *f,const void *b,uint32_t n) {
    if(Write((BPTR)f->handle,(APTR)b,(LONG)n)!=(LONG)n) return 0;
    f->pos+=n; if(f->pos>f->size) f->size=f->pos; return 1;
}
int io_seek(File *f,uint32_t o) {
    if(Seek((BPTR)f->handle,(LONG)o,OFFSET_BEGINNING)<0) return 0;
    f->pos=o; return 1;
}
int io_close(File *f) {
    int ok=1; if(f->handle) ok=Close((BPTR)f->handle)!=0;
    f->handle=0; return ok;
}
int io_exists(const char *p) { BPTR l=Lock((CONST_STRPTR)p,ACCESS_READ); if(!l) return 0; UnLock(l); return 1; }
int io_mkdir(const char *p) { BPTR l=CreateDir((CONST_STRPTR)p); if(!l) return 0; UnLock(l); return 1; }
int io_remove(const char *p) { return DeleteFile((CONST_STRPTR)p)!=0; }
int io_rename(const char *a,const char *b) { return Rename((CONST_STRPTR)a,(CONST_STRPTR)b)!=0; }
void io_message(const char *s) { BPTR out=Output(); if(out) { Write(out,(APTR)s,(LONG)strlen(s)); Write(out,(APTR)"\n",1); } }
/* Freestanding runtime: no libc dependency, hidden buffers or stack swapping. */
void *memcpy(void *d,const void *s,size_t n) { unsigned char *p=d; const unsigned char *q=s; while(n--) *p++=*q++; return d; }
void *memset(void *d,int c,size_t n) { unsigned char *p=d; while(n--) *p++=(unsigned char)c; return d; }
int memcmp(const void *a,const void *b,size_t n) { const unsigned char *p=a,*q=b; while(n--) { if(*p!=*q) return *p-*q; p++; q++; } return 0; }
size_t strlen(const char *s) { const char *p=s; while(*p) p++; return (size_t)(p-s); }
int amiga_main(void) {
    struct Process *process; struct Message *message=0;
    struct RDArgs *args; LONG values[3]={0,0,0}; int result=20;
    __asm__ volatile("move.l 4.w,%0" : "=r"(SysBase));
    process=(struct Process *)FindTask(0);
#ifdef INSTALL_STACK_TEST
    stack_mark(process);
#endif
    if(!process->pr_CLI) {
        WaitPort(&process->pr_MsgPort); message=GetMsg(&process->pr_MsgPort);
        /* This helper is invoked by Installer/CLI, not by opening its own icon. */
    } else if((DOSBase=(struct DosLibrary *)OpenLibrary((CONST_STRPTR)"dos.library",37))) {
        args=ReadArgs((CONST_STRPTR)"ARCHIVE/A,DESTINATION/A,TEMPDIR",values,0);
        if(args) { result=install_data((const char *)values[0],(const char *)values[1],(const char *)(values[2]?values[2]:values[1])); FreeArgs(args); }
        else io_message(INSTALL_USAGE);
        CloseLibrary((struct Library *)DOSBase);
    }
#ifdef INSTALL_STACK_TEST
    stack_check(result);
#endif
    if(message) { Forbid(); ReplyMsg(message); }
    return result;
}
