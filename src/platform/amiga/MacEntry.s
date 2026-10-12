	.text
	.even
	.globl pop2_call_mac
pop2_call_mac:
	move.l 4(sp),a0
	move.l 8(sp),a1
	move.l 12(sp),d0
	movem.l d2-d7/a2-a6,-(sp)
	move.l sp,g_coreHostSP
	move.l a1,a5
	move.l d0,sp
	jsr (a0)
	bra pop2_mac_exit
	.globl pop2_mac_exit
pop2_mac_exit:
	move.l g_coreHostSP,sp
	clr.l g_coreHostSP
	movem.l (sp)+,d2-d7/a2-a6
	rts
	.globl pop2_line_a
pop2_line_a:
	movem.l d0-d7/a0-a6,-(sp)
	move.l sp,a0
	lea 60(sp),a1
	move.l a1,g_coreActiveFrame
	move.l usp,a2
	lea pop2_service_stack_end,sp
	move.l a0,-(sp)
	move.l a2,-(sp)
	move.l a1,-(sp)
	move.l a0,-(sp)
	jsr pop2Dispatch
	lea 12(sp),sp
	move.l (sp)+,a0
	move.l d0,a1
	move.l a1,usp
	clr.l g_coreActiveFrame
	move.l a0,sp
	movem.l (sp)+,d0-d7/a0-a6
	rte
	.globl pop2_old_loadseg
pop2_old_loadseg:
	.word 0xaffe,0xa9f0
	rts
	.globl pop2_old_exit
pop2_old_exit:
	.word 0xaffe,0xa9f4
	rts
	.bss
	.balign 4
pop2_service_stack:
	.space 32768
pop2_service_stack_end:
	.text
	.macro fatal_vector number
	.globl pop2_exception_\number
pop2_exception_\number:
	move.l #\number,g_coreException
	bra pop2_exception_common
	.endm
	fatal_vector 2
	fatal_vector 3
	fatal_vector 4
	fatal_vector 5
	fatal_vector 6
	fatal_vector 7
	fatal_vector 8
	fatal_vector 9
	fatal_vector 11
	fatal_vector 14
pop2_exception_common:
	move.l sp,a0
	lea pop2_fault_stack_end,sp
	move.l a0,-(sp)
	jsr pop2Exception
	move.l d0,sp
	rte
	.bss
	.balign 4
pop2_fault_stack:
	.space 4096
pop2_fault_stack_end:
	.text
	.globl pop2_identity_fixture
	.globl pop2_identity_fixture_end
pop2_identity_fixture:
	lea pop2_identity_selectors(pc),a2
	lea g_coreIdentity,a3
	moveq #6,d7
1:
	move.l (a2)+,d0
	move.l d0,(a3)+
	.word 0xa1ad
	move.l d0,(a3)+
	move.l a0,(a3)+
	dbra d7,1b
	moveq #1,d0
	lea g_coreSysEnv,a0
	.word 0xa090
	move.l d0,g_coreSysEnvError
	rts
	.globl pop2_unknown_fixture
pop2_unknown_fixture:
	.word 0xa123
	rts
	.globl pop2_illegal_fixture
pop2_illegal_fixture:
	.word 0x4afc
	rts
pop2_identity_fixture_end:
pop2_identity_selectors:
	.long 0x766d2020,0x70726f63,0x73797376,0x6d616368,0x6d6d7520,0x61646472,0x612f7578
