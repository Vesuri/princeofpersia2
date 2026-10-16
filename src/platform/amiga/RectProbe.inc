| Verification-only caller of the production Pascal entries, on the real Mac
| user stack. No debugger register or PC injection is involved.
	.ifdef POP2_RECTANGLE_PROBE
	.globl pop2RectProbe
	.globl pop2RectProbeEnd
pop2RectProbe:
	movem.l d0-d7/a0-a6,-(sp)
	move.l sp,g_rectBeforeSP
	move.w #0x7f5a,-(sp)
	cmpi.w #0xa8ae,g_rectTrap
	beq.s 1f
	move.l g_rectArgs+8,-(sp)
	move.l g_rectArgs+4,-(sp)
1:	move.l g_rectArgs,-(sp)
	move.l #0x11220000,d0
	move.l #0x11220101,d1
	move.l #0x11220202,d2
	move.l #0x11220303,d3
	move.l #0x11220404,d4
	move.l #0x11220505,d5
	move.l #0x11220606,d6
	move.l #0x11220707,d7
	move.l #0x22330202,a2
	move.l #0x22330303,a3
	move.l #0x22330404,a4
	move.l #0x22330505,a5
	move.l #0x22330606,a6
	move.l g_rectEntry,a0
	move.w g_rectInitialCCR,ccr
	jsr (a0)
	move.w ccr,g_rectCCR
	movem.l d0-d7/a0-a6,g_rectRegisters
	move.w (sp)+,g_rectResult
	move.l sp,g_rectAfterSP
	movem.l (sp)+,d0-d7/a0-a6
	rts
	| A JSR shim puts the caller return below the trap's Pascal arguments.
	| Move it out, execute the real Line-A instruction, then restore it.
	.macro rect_trap name, word
	.globl \name
\name:
	move.l (sp)+,g_rectTrapReturn
	move.w g_rectInitialCCR,ccr
	.word \word
	move.w ccr,g_rectTrapCCR
	move.l g_rectTrapReturn,-(sp)
	move.w g_rectTrapCCR,ccr
	rts
	.endm
	rect_trap pop2RectTrapSect,0xa8aa
	rect_trap pop2RectTrapUnion,0xa8ab
	rect_trap pop2RectTrapEmpty,0xa8ae
pop2RectProbeEnd:
	.bss
	.balign 4
g_rectTrapReturn: .space 4
g_rectTrapCCR: .space 2
	.text
	.endif
