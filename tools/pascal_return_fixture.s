| Native exception-return ABI oracle. Production cleanup is inserted at build time.
 .text
 .even
 .globl pascalSupervisor
pascalSupervisor:
 movem.l d2-d7/a2-a6,-(sp)
 move.l sp,pascalSavedSP
 move.l usp,a0
 move.l a0,pascalSavedUSP
 move.l pascalGuest,a0
 move.l a0,usp
 lea -8(sp),sp
 move.w pascalCCR+2,d0
 ori.w #0x2000,d0
 move.w d0,(sp)
 move.l #pascalCapture-2,2(sp)
 clr.w 6(sp)
 lea pascalInput,a0
 movem.l (a0),d0-d7/a0-a6
 movem.l d0-d7/a0-a6,-(sp)
 move.l pascalCount,d0
@RETURN@
pascalCapture:
 lea -64(sp),sp
 movem.l d0-d7/a0-a6,(sp)
 move.w ccr,60(sp)
 clr.w 62(sp)
 lea pascalOutput,a0
 move.l usp,a1
 move.l a1,64(a0)
 move.l sp,a1
 moveq #15,d0
1:
 move.l (a1)+,(a0)+
 dbra d0,1b
 move.l pascalSavedUSP,a0
 move.l a0,usp
 move.l pascalSavedSP,sp
 movem.l (sp)+,d2-d7/a2-a6
 rte
 .bss
 .balign 4
pascalSavedSP: .space 4
pascalSavedUSP: .space 4
