#include <exec/execbase.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/dos.h>
struct ExecBase *SysBase;
struct DosLibrary *DOSBase;
typedef struct {unsigned short left,right;} Span;
extern void rowsFixtureCall(const unsigned char*,unsigned char*,unsigned long,unsigned long,unsigned long,unsigned long,Span*,unsigned long*);
static unsigned char pattern(unsigned long i){return (i*37+(i>>8)*13)&255;}
int amiga_main(void){
 static const unsigned short widths[]={0,1,2,3,4,7,8,15,16,17,31,32,33,63,64,65,127,128,129,255,256,257,511,512,639,640};
 unsigned char *src=0,*dst=0,*expected=0;unsigned long regs[11];Span spans[482];
 unsigned long i,c,w,h,sa,da,mode,y,x,extent,n=0;BPTR f=0;int result=20;
 __asm__ volatile("move.l 4.w,%0":"=r"(SysBase));
 DOSBase=(struct DosLibrary*)OpenLibrary((CONST_STRPTR)"dos.library",37);if(!DOSBase)return 20;
 src=AllocMem(327000,MEMF_PUBLIC);dst=AllocMem(327000,MEMF_PUBLIC);expected=AllocMem(327000,MEMF_PUBLIC);
 if(!src||!dst||!expected)goto done;
 for(c=0;c<1666;++c){
  w=c>=1664?640:widths[c/64];sa=(c/4)%4;da=(c/16)%4;mode=c%4;h=c==1665?480:c==1664?0:(c%3==0?1:c%3==1?2:7);
  extent=c==1665?327000:4800;
  for(i=0;i<extent;++i){src[i]=pattern(i);dst[i]=expected[i]=0xa5;}
  for(y=0;y<h;++y)for(x=0;x<w;++x){
   unsigned char v=src[16+sa+y*672+x];
   unsigned long at=16+da+y*680+x;
   dst[at]=v;
   if(mode==1||(mode==2&&x>=w/3&&x<w*2/3)||(mode==3&&(x==0||x+1==w)))dst[at]^=0xff;
   expected[at]=v;
  }
  for(i=0;i<h+2;++i)spans[i].left=spans[i].right=0xa5a5;
  rowsFixtureCall(src+16+sa,dst+16+da,w,h,672,680,spans+1,regs);
  for(i=0;i<11;++i)if(regs[i]!=0x11220000+i*0x101)goto done;
  for(i=0;i<extent;++i)if(dst[i]!=expected[i]||src[i]!=pattern(i))goto done;
  if(spans[0].left!=0xa5a5||spans[0].right!=0xa5a5)goto done;
  for(y=0;y<h;++y){
   unsigned long first=w,last=0;
   for(x=0;x<w;++x)if(mode==1||(mode==2&&x>=w/3&&x<w*2/3)||(mode==3&&(x==0||x+1==w))){if(first==w)first=x;last=x+1;}
   if(spans[y+1].left>spans[y+1].right||spans[y+1].right>w)goto done;
   if(first==w){if(spans[y+1].left!=spans[y+1].right)goto done;}
   else if(spans[y+1].left>first||spans[y+1].right<last||first-spans[y+1].left>3||spans[y+1].right-last>3)goto done;
  }
  for(i=h+1;i<h+2;++i)if(spans[i].left!=0xa5a5||spans[i].right!=0xa5a5)goto done;
  ++n;
 }
 if(n!=1666)goto done;
 f=Open((CONST_STRPTR)"DH1:fixture.complete",MODE_NEWFILE);if(!f)goto done;
 {static const char msg[]="ROWS_COMPLETE cases=1666\n";if(Write(f,(APTR)msg,sizeof(msg)-1)!=sizeof(msg)-1)goto done;}
 result=0;
 done:if(f)Close(f);if(src)FreeMem(src,327000);if(dst)FreeMem(dst,327000);if(expected)FreeMem(expected,327000);CloseLibrary((struct Library*)DOSBase);return result;
}
