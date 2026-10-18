#include <exec/execbase.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "fixture.h"
struct ExecBase *SysBase;struct DosLibrary *DOSBase;
extern void wrapperFixtureCapture(void);
extern void wrapperFixtureInvoke(unsigned long,unsigned long,unsigned long,unsigned long,unsigned long);
static void put16(unsigned char*p,unsigned long v){p[0]=v>>8;p[1]=v;}
static unsigned long get32(const unsigned char*p){return ((unsigned long)p[0]<<24)|((unsigned long)p[1]<<16)|((unsigned long)p[2]<<8)|p[3];}
static void put32(unsigned char*p,unsigned long v){put16(p,v>>16);put16(p+2,v);}
int amiga_main(void){
 unsigned char *arena=0,*oracle=0,*alternative=0,*expected=0,out[68],saved[68],savedFrame[56];
 unsigned long regs[15],i,j,k,cc,which,frame=310272,sp=310016,n=0;BPTR f=0, streams[2]={0,0};unsigned char *delta=0;int result=20;
 __asm__ volatile("move.l 4.w,%0":"=r"(SysBase));
 DOSBase=(struct DosLibrary*)OpenLibrary((CONST_STRPTR)"dos.library",37);if(!DOSBase)return 20;
 arena=AllocMem(311296,MEMF_PUBLIC);expected=AllocMem(307200,MEMF_PUBLIC);oracle=AllocMem(sizeof(original),MEMF_PUBLIC);
 alternative=AllocMem(sizeof(replacement),MEMF_PUBLIC);
 delta=AllocMem(1536004,MEMF_PUBLIC);
 if(!arena||!expected||!oracle||!alternative||!delta)goto done;
 streams[0]=Open((CONST_STRPTR)"DH1:original.effects",MODE_NEWFILE);
 streams[1]=Open((CONST_STRPTR)"DH1:replacement.effects",MODE_NEWFILE);
 if(!streams[0]||!streams[1])goto done;
 for(i=0;i<sizeof(original);++i)oracle[i]=original[i];for(i=0;i<sizeof(replacement);++i)alternative[i]=replacement[i];CacheClearU();
 for(i=0;i<CASE_COUNT;++i)for(cc=0;cc<(i<14?32:1);++cc){
  for(which=0;which<2;++which){
   for(j=0;j<311296;++j)arena[j]=(j*37+i*13)&255;
   put16(arena+frame-18,640);put32(arena+frame-16,(unsigned long)arena+64+(i&3));
   put32(arena+frame-8,(unsigned long)encoded+cases[i][0]);put32(arena+frame-12,(unsigned long)encoded+cases[i][0]+2);
   put16(arena+frame+28,cases[i][4]);put16(arena+frame+16,cases[i][1]);put16(arena+frame+18,cases[i][2]);put16(arena+frame+20,cases[i][3]);
   put32(arena+sp,(unsigned long)wrapperFixtureCapture);
   for(j=0;j<15;++j)regs[j]=0x11220000+j*0x101;regs[0]=0; /* SwapMMUMode returns a full zero/one in D0 before this block. */
   regs[14]=(unsigned long)arena+frame;
   if(i==0 && cc==0){BPTR progress=Open((CONST_STRPTR)"DH1:stage.bin",MODE_NEWFILE);if(!progress)goto done;Write(progress,&which,4);Close(progress);}
   wrapperFixtureInvoke(which?(unsigned long)alternative:(unsigned long)oracle,(unsigned long)arena+sp,(unsigned long)regs,cc,(unsigned long)out);
   if(!which){for(j=0;j<307200;++j)expected[j]=arena[j];for(j=0;j<68;++j)saved[j]=out[j];for(j=0;j<56;++j)savedFrame[j]=arena[frame-24+j];}
   else {for(j=0;j<307200;++j)if(expected[j]!=arena[j])goto done;for(j=0;j<68;++j)if(saved[j]!=out[j])goto done;for(j=0;j<56;++j)if(savedFrame[j]!=arena[frame-24+j])goto done;}
   /* Canonicalize only allocation-dependent addresses. Encode every changed
      framebuffer byte relative to the deterministic initial image, losslessly. */
   put32(out+56,get32(out+56)-(unsigned long)arena);
   put32(out+64,get32(out+64)-(unsigned long)arena);
   put32(arena+frame-16,get32(arena+frame-16)-(unsigned long)arena);
   put32(arena+frame-12,get32(arena+frame-12)-(unsigned long)encoded);
   put32(arena+frame-8,get32(arena+frame-8)-(unsigned long)encoded);
   k=4;
   for(j=0;j<307200;++j)if(arena[j]!=(unsigned char)(j*37+i*13)){
    put32(delta+k,j);delta[k+4]=arena[j];k+=5;
   }
   put32(delta,(k-4)/5);
   if(Write(streams[which],out,68)!=68 || Write(streams[which],arena+frame-24,56)!=56 || Write(streams[which],delta,k)!=k)goto done;
  }
  ++n;
  if(!(n&63)){BPTR progress=Open((CONST_STRPTR)"DH1:progress.bin",MODE_NEWFILE);if(!progress)goto done;Write(progress,&n,4);Close(progress);}
 }
 if(n!=1468)goto done;
 f=Open((CONST_STRPTR)"DH1:fixture.complete",MODE_NEWFILE);if(!f)goto done;
 {static const char msg[]="BANK_SKIP_COMPLETE cases=1468\n";if(Write(f,(APTR)msg,sizeof(msg)-1)!=sizeof(msg)-1)goto done;}
 result=0;
 done:if(streams[0])Close(streams[0]);if(streams[1])Close(streams[1]);if(delta)FreeMem(delta,1536004);if(alternative)FreeMem(alternative,sizeof(replacement));if(f)Close(f);if(arena)FreeMem(arena,311296);if(expected)FreeMem(expected,307200);if(oracle)FreeMem(oracle,sizeof(original));CloseLibrary((struct Library*)DOSBase);return result;
}
