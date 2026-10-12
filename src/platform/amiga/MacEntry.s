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
	| Park the exception identity, then leave supervisor mode before services.
	| DOS may wait and schedule; it must never run on the exception stack.
	move.l a0,-(sp)
	move.w 4(sp),g_coreServiceSR
	move.l 6(sp),g_coreServicePC
	move.l #pop2_user_service,6(sp)
	move.l (sp)+,a0
	rte
pop2_user_service:
	lea -6(sp),sp
	move.w ccr,(sp)
	movem.l d0-d7/a0-a6,-(sp)
	move.l sp,a0
	lea pop2_service_stack_end,sp
	move.l a0,-(sp)
	jsr pop2UserDispatch
	move.l d0,sp
	movem.l (sp)+,d0-d7/a0-a6
	move.w (sp)+,ccr
	rts
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

	.text
	.globl pop2_file_fixture
	.globl pop2_file_fixture_end
pop2_file_fixture:
	move.l g_fileProbePB,a0
	moveq #26,d0
	.word 0xa060
	move.l d0,g_fileProbeResults
	tst.w d0
	bne.s 9f
	.word 0xa002
	move.l d0,g_fileProbeResults+4
	tst.w d0
	bne.s 8f
	tst.l g_fileProbeWriteMode
	beq.s 8f
	move.l g_fileProbeBuffer,a1
	eori.b #1,(a1)
	move.w #1,44(a0)
	clr.l 46(a0)
	.word 0xa003
	move.l d0,g_fileProbeWriteResult
8:
	.word 0xa001
	move.l d0,g_fileProbeResults+8
	move.l g_fileProbeFCB,a0
	moveq #26,d0
	.word 0xa060
	move.l d0,g_fileProbeFCBResults
	tst.w d0
	bne.s 9f
	moveq #8,d0
	.word 0xa260
	move.l d0,g_fileProbeFCBResults+4
	.word 0xa001
	move.l d0,g_fileProbeFCBResults+8
9:	rts
pop2_file_fixture_end:

	.text
	.globl pop2_resource_fixture
	.globl pop2_resource_fixture_end
pop2_resource_fixture:
	lea g_specProbePaths,a2
	lea g_specProbeOutputs,a3
	lea g_specProbeResults,a4
	move.l g_specProbeCount,d7
	subq.l #1,d7
.Lspec_next:
	subq.l #2,sp
	clr.w -(sp)
	clr.l -(sp)
	move.l (a2)+,-(sp)
	move.l (a3)+,-(sp)
	moveq #1,d0
	.word 0xaa52
	move.w (sp)+,(a4)+
	dbra d7,.Lspec_next
	lea g_resourceProbeNames,a2
	lea g_resourceProbeRefs,a3
	moveq #2,d7
1:	subq.l #2,sp
	move.l (a2)+,-(sp)
	.word 0xa997
	move.w (sp)+,(a3)+
	dbra d7,1b
	clr.l -(sp)
	move.l #0x53484150,-(sp)
	move.w #0x61aa,-(sp)
	.word 0xa9a0
	move.l (sp)+,g_resourceProbeHandle
	clr.l -(sp)
	move.l g_resourceProbeHandle,-(sp)
	.word 0xa9a5
	move.l (sp)+,g_resourceProbeSizes
	move.l g_resourceProbeHandle,-(sp)
	.word 0xa992
	subq.l #2,sp
	move.l (a2),-(sp)
	.word 0xa997
	move.w (sp)+,(a3)
	clr.l -(sp)
	move.l #0x44534c56,-(sp)
	move.w #0x1900,-(sp)
	.word 0xa9a0
	move.l (sp)+,g_resourceProbeOption
	clr.l -(sp)
	move.l g_resourceProbeOption,-(sp)
	.word 0xa9a5
	move.l (sp)+,g_resourceProbeSizes+4
	move.l g_resourceProbeOption,-(sp)
	move.l g_resourceProbeInfo,a0
	pea 256(a0)
	pea 260(a0)
	move.l a0,-(sp)
	.word 0xa9a8
	move.l g_resourceProbeOption,-(sp)
	.word 0xa992
	lea g_resourceProbeRefs+8,a3
	moveq #3,d7
2:	move.w -(a3),-(sp)
	.word 0xa99a
	dbra d7,2b
	rts
pop2_resource_fixture_end:

	.text
	.globl pop2_replay_fixture
	.globl pop2_replay_fixture_end
pop2_replay_fixture:
	move.l g_replayBegin,a2
	addq.l #8,a2
	move.l g_replayCount,d7
	subq.l #1,d7
.Lreplay_next:
	lea 16(a2),a0
	btst #4,9(a2)
	beq.s .Lreplay_refs
	move.l g_replayInfoDates,72(a0)
	move.l g_replayInfoDates+4,76(a0)
.Lreplay_refs:
	btst #0,9(a2)
	beq.s .Lreplay_dispatch
	moveq #0,d1
	move.w 10(a2),d1
	add.l d1,d1
	lea g_replayRefs,a1
	move.w (a1,d1.l),24(a0)
.Lreplay_dispatch:
	move.w (a2),d0
	cmp.w #0xa014,d0
	beq .Lreplay_a014
	cmp.w #0xa214,d0
	beq .Lreplay_a214
	cmp.w #0xa015,d0
	beq .Lreplay_a015
	cmp.w #0xa008,d0
	beq .Lreplay_a008
	cmp.w #0xa00c,d0
	beq .Lreplay_a00c
	cmp.w #0xa00d,d0
	beq .Lreplay_a00d
	cmp.w #0xa060,d0
	beq .Lreplay_a060
	cmp.w #0xa001,d0
	beq .Lreplay_a001
	cmp.w #0xa002,d0
	beq .Lreplay_a002
	cmp.w #0xa003,d0
	beq .Lreplay_a003
	cmp.w #0xa011,d0
	beq .Lreplay_a011
	cmp.w #0xa013,d0
	beq .Lreplay_a013
	cmp.w #0xa200,d0
	beq .Lreplay_a200
	cmp.w #0xa260,d0
	beq .Lreplay_a260
	cmp.w #0xa044,d0
	beq .Lreplay_a044
	cmp.w #0xa207,d0
	beq .Lreplay_a207
	.word 0xa123
.Lreplay_a014:
	moveq #0,d0
	move.w 2(a2),d0
	.word 0xa014
	bra .Lreplay_return
.Lreplay_a214:
	moveq #0,d0
	move.w 2(a2),d0
	.word 0xa214
	bra .Lreplay_return
.Lreplay_a015:
	moveq #0,d0
	move.w 2(a2),d0
	.word 0xa015
	bra .Lreplay_return
.Lreplay_a008:
	moveq #0,d0
	move.w 2(a2),d0
	.word 0xa008
	bra .Lreplay_return
.Lreplay_a00c:
	moveq #0,d0
	move.w 2(a2),d0
	.word 0xa00c
	bra .Lreplay_return
.Lreplay_a00d:
	moveq #0,d0
	move.w 2(a2),d0
	.word 0xa00d
	bra .Lreplay_return
.Lreplay_a060:
	moveq #0,d0
	move.w 2(a2),d0
	.word 0xa060
	bra .Lreplay_return
.Lreplay_a001:
	moveq #0,d0
	move.w 2(a2),d0
	.word 0xa001
	bra .Lreplay_return
.Lreplay_a002:
	moveq #0,d0
	move.w 2(a2),d0
	.word 0xa002
	bra .Lreplay_return
.Lreplay_a003:
	moveq #0,d0
	move.w 2(a2),d0
	.word 0xa003
	bra .Lreplay_return
.Lreplay_a011:
	moveq #0,d0
	move.w 2(a2),d0
	.word 0xa011
	bra .Lreplay_return
.Lreplay_a013:
	moveq #0,d0
	move.w 2(a2),d0
	.word 0xa013
	bra .Lreplay_return
.Lreplay_a200:
	moveq #0,d0
	move.w 2(a2),d0
	.word 0xa200
	bra .Lreplay_return
.Lreplay_a260:
	moveq #0,d0
	move.w 2(a2),d0
	.word 0xa260
	bra .Lreplay_return
.Lreplay_a044:
	moveq #0,d0
	move.w 2(a2),d0
	.word 0xa044
	bra .Lreplay_return
.Lreplay_a207:
	moveq #0,d0
	move.w 2(a2),d0
	.word 0xa207
	bra .Lreplay_return
.Lreplay_return:
	move.l d0,12(a2)
	tst.w d0
	bne.s .Lreplay_advance
	cmp.w #0xa00c,(a2)
	bne.s .Lreplay_open_ref
	move.l 72(a0),g_replayInfoDates
	move.l 76(a0),g_replayInfoDates+4
.Lreplay_open_ref:
	btst #1,9(a2)
	beq.s .Lreplay_advance
	moveq #0,d1
	move.w 10(a2),d1
	add.l d1,d1
	lea g_replayRefs,a1
	move.w 24(a0),(a1,d1.l)
.Lreplay_advance:
	addq.l #1,g_replayCompleted
	adda.l 4(a2),a2
	dbra d7,.Lreplay_next
	rts
pop2_replay_fixture_end:

    .globl pop2_display_fixture,pop2_display_fixture_end
pop2_display_fixture:
    move.l a2,-(sp)
    move.l g_displayServiceProbe,a2
    pea 202(a2)
    .word 0xa86e
    clr.l -(sp)
    .word 0xaa2a
    move.l (sp)+,212(a2)
    clr.w -(sp)
    pea 216(a2)
    move.w #8,-(sp)
    pea 224(a2)
    clr.l -(sp)
    move.l 212(a2),-(sp)
    move.l #2,-(sp)
    moveq #0,d0
    .word 0xab1d
    move.w (sp)+,232(a2)
    move.l 216(a2),a0
    move.l 2(a0),220(a2)
    clr.w -(sp)
    move.l 220(a2),-(sp)
    moveq #1,d0
    .word 0xab1d
    move.w (sp)+,234(a2)
    clr.l -(sp)
    move.l 220(a2),-(sp)
    moveq #15,d0
    .word 0xab1d
    move.l (sp)+,236(a2)
    move.l 220(a2),-(sp)
    moveq #2,d0
    .word 0xab1d
    clr.l -(sp)
    move.l 220(a2),-(sp)
    moveq #15,d0
    .word 0xab1d
    move.l (sp)+,240(a2)
    move.l (sp)+,a2
    rts
    .globl pop2_display_rejection_fixture
pop2_display_rejection_fixture:
    clr.l -(sp)
    move.l #0x7fff,d0
    .word 0xab1d
    rts
pop2_display_fixture_end:

    .globl pop2_display_scene_init,pop2_display_scene_frame,pop2_display_scene_end
pop2_display_scene_init:
    movem.l d3/a2,-(sp)
    clr.l -(sp)
    move.w #400,-(sp)
    clr.l -(sp)
    move.l #-1,-(sp)
    .word 0xaa46
    move.l (sp)+,g_displayWindow
    move.l g_displayWindow,-(sp)
    .word 0xa873
    move.w #65,-(sp)
    move.w #48,-(sp)
    .word 0xa877
    clr.l -(sp)
    move.w #256,-(sp)
    clr.l -(sp)
    move.w #12,-(sp)
    clr.w -(sp)
    .word 0xaa91
    move.l (sp)+,g_displayPalette
    moveq #0,d3
.Lscene_entry:
    move.l g_displaySceneColors,a2
    move.l (a2),a2
    move.l d3,d0
    lsl.l #3,d0
    lea 10(a2,d0.l),a2
    move.l g_displayPalette,-(sp)
    move.w d3,-(sp)
    move.l a2,-(sp)
    .word 0xaa9c
    addq.w #1,d3
    cmp.w #256,d3
    bne .Lscene_entry
    move.l g_displayPalette,-(sp)
    clr.w -(sp)
    move.w #2,-(sp)
    clr.w -(sp)
    .word 0xaa9e
    move.l g_displayPalette,-(sp)
    move.w #255,-(sp)
    move.w #2,-(sp)
    clr.w -(sp)
    .word 0xaa9e
    move.l g_displayWindow,-(sp)
    move.l g_displayPalette,-(sp)
    move.w #0x0100,-(sp)
    .word 0xaa95
    move.l g_displayWindow,-(sp)
    .word 0xa915
    move.l g_displayServiceProbe,a2
    clr.w -(sp)
    move.l 220(a2),-(sp)
    moveq #1,d0
    .word 0xab1d
    addq.l #2,sp
    clr.l -(sp)
    move.l 220(a2),-(sp)
    moveq #15,d0
    .word 0xab1d
    move.l (sp)+,236(a2)
    movem.l (sp)+,d3/a2
    rts
pop2_display_scene_frame:
    move.l a2,-(sp)
    move.l g_displayWindow,-(sp)
    move.l g_displaySceneColors,-(sp)
    move.w #1,-(sp)
    move.w #1,-(sp)
    move.w #254,-(sp)
    .word 0xaa9a
    move.l g_displayServiceProbe,a2
    move.l 220(a2),a0
    move.l (a0),-(sp)
    move.l g_displayWindow,a0
    addq.l #2,a0
    move.l a0,-(sp)
    pea 224(a2)
    pea 224(a2)
    clr.w -(sp)
    clr.l -(sp)
    .word 0xa8ec
    move.l (sp)+,a2
    rts
pop2_display_scene_end:
