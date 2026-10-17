/* Standalone original/replacement differential; never linked into release. */
#include <exec/execbase.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "fixture.h"
struct ExecBase *SysBase;
struct DosLibrary *DOSBase;
extern void pop2SaveRectRow(void),wrapperFixtureCapture(void);
extern void wrapperFixtureInvoke(unsigned long,unsigned long,unsigned long,unsigned long,unsigned long);
static void put32(unsigned char *p,unsigned long v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static unsigned long get32(const unsigned char *p){return ((unsigned long)p[0]<<24)|((unsigned long)p[1]<<16)|((unsigned long)p[2]<<8)|p[3];}
int amiga_main(void){
 unsigned char *arena=0,*oracle=0,out[68],row[68];unsigned long regs[15];
 unsigned long i,j,k,which,n,frame,sp,width,src,dst;BPTR f=0;int result=20;
 __asm__ volatile("move.l 4.w,%0":"=r"(SysBase));
 DOSBase=(struct DosLibrary*)OpenLibrary((CONST_STRPTR)"dos.library",37);if(!DOSBase)return 20;
 arena=AllocMem(0x12000,MEMF_PUBLIC);oracle=AllocMem(sizeof(original),MEMF_PUBLIC);
 if(!arena||!oracle)goto done;
 for(i=0;i<sizeof(original);++i)oracle[i]=original[i];CacheClearU();
 for(which=0;which<2;++which){
  f=Open((CONST_STRPTR)(which?"DH1:replacement.bin":"DH1:original.bin"),MODE_NEWFILE);if(!f)goto done;
  for(i=0;i<CASE_COUNT;++i){
   width=cases[i][0];src=cases[i][1];dst=cases[i][2];n=width+(src>dst?src:dst)+16;
   frame=(n+255)&~3UL;sp=frame+128;
   for(j=0;j<sp+128;++j)arena[j]=(j*37+i*13)&255;
   put32(arena+frame-32,(unsigned long)arena+src);put32(arena+frame-36,(unsigned long)arena+dst);
   put32(arena+frame-48,0);put32(arena+frame-8,width);
   put32(arena+sp,(unsigned long)wrapperFixtureCapture);
   for(j=0;j<15;++j)regs[j]=0x11220000+j*0x101;
   regs[0]=0;regs[3]=width;regs[14]=(unsigned long)arena+frame;
   wrapperFixtureInvoke(which?(unsigned long)pop2SaveRectRow:(unsigned long)oracle,
    (unsigned long)arena+sp,(unsigned long)regs,cases[i][3],(unsigned long)out);
   for(j=0;j<68;++j)row[j]=out[j];
   /* Normalize only known pointers; compare all 15 registers, CCR and SP. */
   for(j=0;j<3;++j){k=j==0?32:j==1?56:64;put32(row+k,get32(row+k)-(unsigned long)arena);}
   put32(arena+frame-32,get32(arena+frame-32)-(unsigned long)arena);
   put32(arena+frame-36,get32(arena+frame-36)-(unsigned long)arena);
   if(Write(f,row,68)!=68||Write(f,arena,n)!=(LONG)n||Write(f,arena+frame-64,64)!=64)goto done;
  }
  Close(f);f=0;
 }
 f=Open((CONST_STRPTR)"DH1:fixture.complete",MODE_NEWFILE);if(!f)goto done;
 {const char msg[]="SAVE_RECT_COMPLETE\n";if(Write(f,(APTR)msg,sizeof(msg)-1)!=sizeof(msg)-1)goto done;}
 result=0;
 done:if(f)Close(f);if(arena)FreeMem(arena,0x12000);if(oracle)FreeMem(oracle,sizeof(original));CloseLibrary((struct Library*)DOSBase);return result;
}
