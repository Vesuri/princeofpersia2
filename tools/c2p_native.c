/* Standalone byte/ABI oracle. Never linked into the game. */
#include <exec/execbase.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/dos.h>
struct ExecBase *SysBase;
struct DosLibrary *DOSBase;
extern void c2pFixtureCall(const unsigned char*,unsigned char*,unsigned long,unsigned long,unsigned long*);
static unsigned char pattern(unsigned long i){return (i*37+(i>>8)*13)&255;}
int amiga_main(void){
 unsigned char *src=0,*dst=0,*expected=0;unsigned long regs[11];
 unsigned long i,c,width,rows,top,left,offset,y,x,p,n=0;BPTR f=0;int result=20;
 __asm__ volatile("move.l 4.w,%0":"=r"(SysBase));
 DOSBase=(struct DosLibrary*)OpenLibrary((CONST_STRPTR)"dos.library",37);if(!DOSBase)return 20;
 src=AllocMem(307204,MEMF_PUBLIC);dst=AllocMem(196640,MEMF_PUBLIC);expected=AllocMem(196640,MEMF_PUBLIC);
 if(!src||!dst||!expected)goto done;
 for(i=0;i<307204;++i)src[i]=pattern(i);
 for(c=0;c<193;++c){
  width=c==192?512:32*(1+c/12);rows=c==192?384:(c%3==0?1:c%3==1?2:7);
  top=c==192?0:((c&2)?384-rows:0);left=c==192?0:((width<512&&(c&1))?16:0);
  offset=c==192?96*640+128:((c&1)?480-rows:0)*640+(c/3)%4;
  for(i=0;i<196640/4;++i)((unsigned long*)dst)[i]=((unsigned long*)expected)[i]=0xa5a5a5a5;
  for(y=0;y<rows;++y)for(x=0;x<width;++x){
   unsigned char value=src[offset+y*640+x],mask=128>>((left+x)&7);
   for(p=0;p<8;++p){unsigned long at=16+(top+y)*512+p*64+(left+x)/8;
    if(value&(1<<p))expected[at]|=mask;else expected[at]&=~mask;
   }
  }
  c2pFixtureCall(src+offset,dst+16+top*512+left/8,width,rows,regs);
  for(i=0;i<11;++i)if(regs[i]!=0x11220000+i*0x101)goto done;
  for(i=0;i<196640/4;++i)if(((unsigned long*)dst)[i]!=((unsigned long*)expected)[i])goto done;
  ++n;
 }
 for(i=0;i<307204;++i)if(src[i]!=pattern(i))goto done;
 if(n!=193)goto done;
 f=Open((CONST_STRPTR)"DH1:fixture.complete",MODE_NEWFILE);if(!f)goto done;
 {static const char msg[]="C2P_COMPLETE cases=193\n";if(Write(f,(APTR)msg,sizeof(msg)-1)!=sizeof(msg)-1)goto done;}
 result=0;
 done:if(f)Close(f);if(src)FreeMem(src,307204);if(dst)FreeMem(dst,196640);if(expected)FreeMem(expected,196640);CloseLibrary((struct Library*)DOSBase);return result;
}
