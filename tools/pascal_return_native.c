/* Exercise the actual return sequence with every CCR and Pascal cleanup size. */
#include <exec/execbase.h>
#include <proto/exec.h>
#include <proto/dos.h>
struct ExecBase *SysBase;
struct DosLibrary *DOSBase;
extern unsigned long pascalSupervisor(void);
unsigned long pascalInput[15],pascalOutput[17],pascalGuest,pascalCount,pascalCCR;
static unsigned long guest[32];
int amiga_main(void){
 unsigned long i,j,c,n=0;BPTR f=0;int result=20;
 static const unsigned short counts[]={0,2,4,8,12,22};
 __asm__ volatile("move.l 4.w,%0":"=r"(SysBase));
 DOSBase=(struct DosLibrary*)OpenLibrary((CONST_STRPTR)"dos.library",37);if(!DOSBase)return 20;
 for(i=0;i<15;++i)pascalInput[i]=0x12340000+i*0x101;
 pascalGuest=(unsigned long)(guest+8);
 Forbid();
 for(c=0;c<6;++c)for(j=0;j<32;++j){
  for(i=0;i<32;++i)guest[i]=0xa5a5a5a5;
  pascalCount=counts[c]+1;pascalCCR=j;
  Supervisor(pascalSupervisor);
  for(i=0;i<15;++i)if(pascalOutput[i]!=pascalInput[i])goto tested;
  if(pascalOutput[15]!=(j<<16) || pascalOutput[16]!=pascalGuest+counts[c])goto tested;
  for(i=0;i<32;++i)if(guest[i]!=0xa5a5a5a5)goto tested;
  ++n;
 }
 tested:Permit();
 if(n!=192)goto done;
 f=Open((CONST_STRPTR)"DH1:fixture.complete",MODE_NEWFILE);if(!f)goto done;
 {static const char msg[]="PASCAL_COMPLETE cases=192\n";if(Write(f,(APTR)msg,sizeof(msg)-1)!=sizeof(msg)-1)goto done;}
 result=0;
 done:if(f)Close(f);CloseLibrary((struct Library*)DOSBase);return result;
}
