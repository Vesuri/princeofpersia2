/* Standalone diagnostic fixture; never linked into the game. */
#include <exec/execbase.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "fixture.h"

struct ExecBase *SysBase;
struct DosLibrary *DOSBase;
extern void wrapperFixtureCapture(void);
extern void wrapperFixtureInvoke(unsigned long entry, unsigned long stack,
                                unsigned long registers, unsigned long ccr,
                                unsigned long output);
static unsigned long read32(const unsigned char *p) {
    return ((unsigned long)p[0]<<24)|((unsigned long)p[1]<<16)|((unsigned long)p[2]<<8)|p[3];
}
int amiga_main(void) {
    unsigned char *code=0,*stack=0,output[68];
    unsigned long *rows=0,registers[15];
    unsigned long i,which,pattern,cc,index=0;BPTR file=0;int result=20;
    __asm__ volatile("move.l 4.w,%0":"=r"(SysBase));
    DOSBase=(struct DosLibrary *)OpenLibrary((CONST_STRPTR)"dos.library",37);
    if(!DOSBase)return 20;
    code=AllocMem(sizeof(fixture_image),MEMF_PUBLIC|MEMF_CLEAR);
    stack=AllocMem(8192,MEMF_PUBLIC|MEMF_CLEAR);
    rows=AllocMem(640*23*4,MEMF_PUBLIC|MEMF_CLEAR);
    if(!code || !stack || !rows)goto done;
    for(i=0;i<sizeof(fixture_image);++i)code[i]=fixture_image[i];
    for(i=0;i<sizeof(fixture_relocations)/sizeof(fixture_relocations[0]);++i)
        *(unsigned long *)(code+fixture_relocations[i])+=(unsigned long)code;
    for(i=0;i<10;++i)*(unsigned long *)(code+fixture_targets[i])=(unsigned long)wrapperFixtureCapture;
    CacheClearU();
    for(which=0;which<10;++which)for(pattern=0;pattern<2;++pattern)for(cc=0;cc<32;++cc) {
        unsigned long seed=pattern?0x89ab0000:0x11220000;
        unsigned char *sp=stack+4096;unsigned long *row=rows+index*23;
        unsigned long argument,intact=1;
        for(i=0;i<15;++i)registers[i]=seed+i*0x101;
        for(i=0;i<64;++i)sp[i]=i^0xa5;
        *(unsigned long *)(code+FIXTURE_HOOK)=0xffffffff;
        *(unsigned long *)(code+FIXTURE_ARGUMENT)=0xffffffff;
        wrapperFixtureInvoke((unsigned long)(code+fixture_wrappers[which]),
            (unsigned long)sp,(unsigned long)registers,cc,(unsigned long)output);
        row[0]=which;row[1]=pattern;row[2]=cc;
        for(i=0;i<15;++i)row[3+i]=read32(output+4*i);
        row[18]=(((unsigned long)output[60]<<8)|output[61])&31;
        row[19]=read32(output+64)-(unsigned long)sp;
        argument=*(unsigned long *)(code+FIXTURE_ARGUMENT);
        if(which==0 || which==8)argument-=(unsigned long)sp;
        else if(which!=5 && which!=6 && which!=7)argument=0;
        row[20]=argument;row[21]=*(unsigned long *)(code+FIXTURE_HOOK);
        for(i=0;i<64;++i)if(sp[i]!=(i^0xa5))intact=0;
        row[22]=intact;++index;
    }
    file=Open((CONST_STRPTR)"DH1:wrapper-rows.bin",MODE_NEWFILE);
    if(!file)goto done;
    if(Write(file,rows,640*23*4)!=640*23*4)goto done;
    Close(file);file=0;
    file=Open((CONST_STRPTR)"DH1:fixture.complete",MODE_NEWFILE);
    if(!file)goto done;
    { const char message[]="WRAPPER_FIXTURE_COMPLETE cases=640\n";
      if(Write(file,(APTR)message,sizeof(message)-1)!=sizeof(message)-1)goto done; }
    result=0;
done:
    if(file)Close(file);
    if(rows)FreeMem(rows,640*23*4);
    if(stack)FreeMem(stack,8192);
    if(code)FreeMem(code,sizeof(fixture_image));
    CloseLibrary((struct Library *)DOSBase);
    return result;
}
