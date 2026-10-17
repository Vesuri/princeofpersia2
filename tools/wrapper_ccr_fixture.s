| Authored ABI fixture. Link the actual MacEntry.s integration section against
| deliberately destructive callbacks; no original game instructions are copied.
	.section .text.wrapper_fixture,"ax"
	.globl wrapperFixtureDone
wrapperFixtureDone:
	nop
	rts
	.macro hook name,id
	.globl \name
\name:
	move.l #\id,wrapperFixtureHook
	bra.w wrapperFixtureClobber
	.endm
	hook pop2IntegrationTitleHook,0
	hook pop2IntegrationOpeningHook,1
	hook pop2IntegrationRoomHook,2
	hook pop2GameplayRouteHall,3
	hook pop2GameplayRouteEnding,4
	hook pop2GameplayRouteFrame,5
	hook pop2GameplaySaved,6
	hook pop2GameplayLoaded,7
	hook pop2IntegrationDeathHook,8
	hook pop2IntegrationDeathReturned,9
wrapperFixtureClobber:
	move.l 4(sp),wrapperFixtureArgument
	movem.l wrapperFixtureClobbers,d0-d7/a0-a6
	move.w #31,ccr
	rts
	.section .data.wrapper_fixture,"aw"
	.balign 4
	.macro target name
	.globl \name
\name:
	.long wrapperFixtureDone
	.endm
	target g_integrationTitleTarget
	target g_integrationOpeningReturn
	target g_integrationRoomReturn
	target g_routeHallReturn
	target g_routeEndingReturn
	target g_routeReturn
	target g_gameplaySaveReturn
	target g_gameplayLoadReturn
	target g_integrationDeathTarget
	target g_integrationDeathReturn
	.globl wrapperFixtureHook,wrapperFixtureArgument
wrapperFixtureHook: .long -1
wrapperFixtureArgument: .long -1
wrapperFixtureClobbers:
	.rept 15
	.long 0xdeadbeef
	.endr

| Native-only invocation bridge. The loaded fixture uses a private stack and
| arbitrary application registers. Capture immediately after its actual RTS,
| before any flag-changing instruction, then restore the C caller's stack.
	.section .text.wrapper_native,"ax"
	.globl wrapperFixtureInvoke,wrapperFixtureCapture
wrapperFixtureInvoke:
	movem.l d2-d7/a2-a6,-(sp)
	move.l sp,wrapperFixtureSavedSP
	move.l 64(sp),wrapperFixtureOutput
	move.l 52(sp),a0
	move.l 48(sp),-4(a0)
	move.w 62(sp),-6(a0)
	move.l 56(sp),a1
	lea -4(a0),sp
	movem.l (a1),d0-d7/a0-a6
	move.w -2(sp),ccr
	rts
wrapperFixtureCapture:
	lea -64(sp),sp
	movem.l d0-d7/a0-a6,(sp)
	move.w ccr,60(sp)
	clr.w 62(sp)
	move.l wrapperFixtureOutput,a0
	lea 64(sp),a1
	move.l a1,64(a0)
	move.l sp,a1
	moveq #15,d0
1:
	move.l (a1)+,(a0)+
	dbra d0,1b
	move.l wrapperFixtureSavedSP,sp
	movem.l (sp)+,d2-d7/a2-a6
	rts
	.section .bss.wrapper_native,"aw",@nobits
	.balign 4
wrapperFixtureSavedSP: .space 4
wrapperFixtureOutput: .space 4
