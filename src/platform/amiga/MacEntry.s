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
	| Table-selected nonblocking services, following Alone in the Dark's
	| complete register-image ABI. Misses restore the original exception.
	movem.l d0-d7/a0-a6,-(sp)
	.ifdef POP2_INTEGRATION_PROBE
	| Startup observers install original-return hooks in the full dispatcher.
	| Keep those diagnostic safe points regardless of quick-service cadence.
	cmpi.l #3,g_integrationStage
	bcs.w .quick_miss
	.endif
	lea 60(sp),a1
	move.l 2(a1),a0
	moveq #0,d0
	move.w (a0),d0
	andi.w #0x0fff,d0
	lea g_pop2QuickTraps,a0
	move.l (a0,d0.w*4),d0
	beq.s .quick_miss
	bclr #0,d0
	move.l d0,a3
	bne.w .native_quick
.quick_c_bridge:
	lea 60(sp),a1
	move.l sp,a0
	move.l usp,a2
	move.l a2,-(sp)
	move.l a1,-(sp)
	move.l a0,-(sp)
	jsr (a3)
	lea 12(sp),sp
	tst.l d0
	beq.s .quick_miss
.quick_done:
	addq.l #2,62(sp)
	movem.l (sp)+,d0-d7/a0-a6
	rte
.native_quick:
	jmp (a3)
.quick_miss:
	movem.l (sp)+,d0-d7/a0-a6
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
	.ifdef POP2_INPUT
	.globl pop2QuickSystemTask
pop2QuickSystemTask:
	moveq #0,d2
	bra.s pop2QuickNativeGuard
	.globl pop2QuickTickCount
pop2QuickTickCount:
	moveq #1,d2
	bra.s pop2QuickNativeGuard
	.globl pop2QuickStripAddress
pop2QuickStripAddress:
	lea pop2QuickStripAddressWarm,a4
	moveq #2,d2
	bra.s pop2QuickNativeGuard
	.globl pop2QuickSwapMMUMode
pop2QuickSwapMMUMode:
	lea pop2QuickSwapMMUModeWarm,a4
	moveq #3,d2
	bra.s pop2QuickNativeGuard
	.globl pop2QuickHLock
pop2QuickHLock:
	lea pop2QuickHLockWarm,a4
	moveq #4,d2
	bra.s pop2QuickNativeGuard
	.globl pop2QuickHGetState
pop2QuickHGetState:
	lea pop2QuickHGetStateWarm,a4
	moveq #5,d2
	.globl pop2QuickNativeGuard
pop2QuickNativeGuard:
	| Validate ownership before reusing or refreshing a no-due certificate.
	| Pending services and failed checks retain the C/ordinary path.
	btst #5,60(sp)
	bne.w .quick_miss
	move.l usp,a0
	cmpa.l g_quickStackLow,a0
	bcs.w .quick_miss
	cmpa.l g_quickStackHigh,a0
	bhi.w .quick_miss
	move.l 62(sp),d0
	.ifdef POP2_TIMING_PROBE
	cmpi.w #1,d2
	bne.s .quick_probe_done
	move.l g_coreSegments+136,d1
	addi.l #0x313e,d1
	cmp.l d1,d0
	beq.w .quick_miss
.quick_probe_done:
	.endif
	| Segment installation invalidates this previously validated range.
	cmp.l g_quickCodeBase,d0
	bcs.s .quick_owner_row
	cmp.l g_quickCodeEnd,d0
	bls.w .quick_owner_valid
.quick_owner_row:
	| Keep a candidate per service so unrelated CODE callers do not evict it.
	| Only validated g_coreSegments rows enter this table; read live bounds.
	lea g_quickOwnerRows,a3
	lea (a3,d2.w*4),a3
	move.l (a3),a0
	cmpa.l #0,a0
	beq.s .quick_owner_cache
	move.l (a0),d1
	cmp.l d1,d0
	bcs.s .quick_owner_cache
	move.l 4(a0),d3
	cmpi.l #2,d3
	bcs.s .quick_owner_cache
	add.l d1,d3
	subq.l #2,d3
	cmp.l d3,d0
	bls.s .quick_owner_record
.quick_owner_cache:
	| Shared CodeRangeCache candidates are index+1, not ownership proofs.
	| Recheck the index and live [base,size] before accepting a native hit.
	move.l d0,d1
	lsr.l #8,d1
	lsr.l #4,d1
	andi.w #255,d1
	move.l g_quickCodeIndexes,a0
	move.l (a0,d1.w*4),d1
	beq.s .quick_owner
	cmpi.l #24,d1
	bhi.s .quick_owner
	subq.l #1,d1
	lea g_coreSegments,a0
	lea (a0,d1.w*8),a0
	move.l (a0),d1
	cmp.l d1,d0
	bcs.s .quick_owner
	move.l 4(a0),d3
	cmpi.l #2,d3
	bcs.s .quick_owner
	add.l d1,d3
	subq.l #2,d3
	cmp.l d3,d0
	bhi.s .quick_owner
	move.l a0,(a3)
.quick_owner_record:
	move.l d1,g_quickCodeBase
	move.l d3,g_quickCodeEnd
	bra.s .quick_owner_valid
.quick_owner:
	move.l d0,-(sp)
	jsr pop2QuickLocate
	addq.l #4,sp
	tst.l d0
	beq.w .quick_miss
	move.l d0,(a3)
.quick_owner_valid:
	| The shared owner lookup may clobber caller-saved a0.
	move.l usp,a0
	move.l displayBatchFrame,d0
	beq.s 1f
	cmp.l a0,d0
	bcs.w .quick_miss
	move.l d0,a0
	move.l displayBatchParent,d0
	cmp.l (a0),d0
	bne.w .quick_miss
	move.l displayBatchReturn,d0
	cmp.l 4(a0),d0
	bne.w .quick_miss
1:
	tst.l g_quickServicesValid
	beq.s .quick_ready
	cmpi.w #1,d2
	beq.s 3f
	jsr pop2ServiceTicks
	bra.s 4f
3:
	jsr pop2WallTicks
4:
	tst.l g_clockFault
	bne.w .quick_miss
	cmp.l g_quickServicesTick,d0
	bne.s .quick_ready
	move.l g_scaffoldFields,d1
	cmp.l g_quickServicesField,d1
	bne.s .quick_ready
	cmpi.l #-1,g_quickServicesMicro
	beq.s .quick_prepared
	move.l d0,d3
	jsr pop2QuickDeadlineReady
	tst.l d0
	beq.w .quick_miss
	move.l d3,d0
	bra.s .quick_prepared
.quick_ready:
	move.l d2,-(sp)
	jsr pop2QuickReady
	addq.l #4,sp
	tst.l d0
	beq.w .quick_miss
	move.l g_quickServicesTick,d0
.quick_prepared:
	.ifdef POP2_VERIFY
	move.l d0,d3
	move.l sp,a0
	lea 60(sp),a1
	move.l usp,a2
	move.l a2,-(sp)
	move.l a1,-(sp)
	move.l a0,-(sp)
	jsr pop2QuickVerify
	lea 12(sp),sp
	tst.l d0
	beq.w .quick_miss
	move.l d3,d0
	.endif
	tst.w d2
	beq.s 2f
	cmpi.w #1,d2
	bne.s 5f
	move.l usp,a0
	move.l d0,(a0)
	clr.l (sp)
	bra.w .quick_done
2:
	move.l g_quickA5,a0
	move.l (a0),(sp)
	bra.w .quick_done
5:
	move.l sp,a0
	lea 60(sp),a1
	move.l a1,-(sp)
	move.l a0,-(sp)
	jsr (a4)
	addq.l #8,sp
	tst.l d0
	beq.w .quick_miss
	bra.w .quick_done
	.endif
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
	move.w g_resourceProbeID,-(sp)
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

	| Authored stable entry points for measured GetTrapAddress queries.
	| The dispatcher removes the JSR return before applying Pascal arguments.
	.even
	.globl pop2_cached_services
	.globl pop2_cached_services_end
pop2_cached_services:
	.word 0xaffe,0xa8aa
	.word 0xaffe,0xa8ab
	.word 0xaffe,0xa8ae
	.word 0xaffe,0xa87b
	.word 0xaffe,0xa976
	.word 0xaffe,0xa89f
	.word 0xaffe,0xa9ad
	.word 0xaffe,0xaa2a
	.word 0xaffe,0xa860
	.word 0xaffe,0xa800
	.word 0xaffe,0xa090
	.word 0xaffe,0xa1ad
pop2_cached_services_end:

	| Direct Pascal builtins. Keep original argument slots and Boolean padding;
	| saved C-clobbered registers are restored without an argument memmove.
	.macro pure_rect name,body,bytes,fallback
	.globl \name
	.type \name,@function
\name:
	move.w ccr,-(sp)
	movem.l d0-d1/a0-a1,-(sp)
	move.l sp,a0
	move.l a0,-(sp)
	pea 22(a0)
	jsr \body
	addq.l #8,sp
	tst.l d0
	bmi.s 1f
	move.w d0,16(sp)
	move.l 18(sp),18+\bytes(sp)
	movem.l (sp)+,d0-d1/a0-a1
	move.w (sp)+,ccr
	lea \bytes(sp),sp
	rts
1:	movem.l (sp)+,d0-d1/a0-a1
	move.w (sp)+,ccr
	jmp pop2_cached_services+\fallback
	.endm
	pure_rect pop2SectRect,pop2SectRectBody,12,0
	pure_rect pop2UnionRect,pop2UnionRectBody,12,4
	pure_rect pop2EmptyRect,pop2EmptyRectBody,4,8

	.globl pop2_ui_fixture
	.globl pop2_ui_fixture_end
pop2_ui_fixture:
	move.l g_uiText,a0
	pea 32(a0)
	.word 0xa976
	move.l g_uiText,a0
	pea 48(a0)
	jsr pop2_cached_services+16
	lea g_uiReport,a2
	subq.l #4,sp
	move.w #'n',-(sp)
	.word 0xa93e
	move.l (sp)+,(a2)+
	subq.l #4,sp
	move.w #'s',-(sp)
	.word 0xa93e
	move.l (sp)+,(a2)+
	move.l -0x5310(a5),-(sp)
	move.w #5,-(sp)
	.word 0xa939
	subq.l #4,sp
	move.w #'s',-(sp)
	.word 0xa93e
	move.l (sp)+,(a2)+
	move.l -0x5310(a5),-(sp)
	move.w #5,-(sp)
	.word 0xa93a
	subq.l #4,sp
	move.w #'s',-(sp)
	.word 0xa93e
	move.l (sp)+,(a2)+
	subq.l #4,sp
	move.w #'q',-(sp)
	.word 0xa93e
	move.l (sp)+,(a2)+
	subq.l #4,sp
	move.w #'t',-(sp)
	.word 0xa93e
	move.l (sp)+,(a2)+
	subq.l #4,sp
	move.w #'m',-(sp)
	.word 0xa93e
	move.l (sp)+,(a2)+
	subq.l #4,sp
	move.w #'h',-(sp)
	.word 0xa93e
	move.l (sp)+,(a2)+
	| The fixed-display action cannot be enabled by an original menu update.
	move.l -0x5318(a5),-(sp)
	move.w #4,-(sp)
	.word 0xa939
	subq.l #4,sp
	move.w #'f',-(sp)
	.word 0xa93e
	move.l (sp)+,(a2)+
	move.l -0x5318(a5),-(sp)
	move.w #7,-(sp)
	.word 0xa939
	move.l -0x5318(a5),a0
	move.l (a0),a0
	move.l 10(a0),(a2)+
	clr.w -0x56d8(a5)
	clr.w -0x4ffe(a5)
	move.l g_coreSegments+16,a0
	lea 0x1a36(a0),a0
	jsr (a0)
	moveq #0,d0
	move.w -0x56d8(a5),d0
	move.l d0,(a2)+
	move.l -0x5318(a5),a0
	move.l (a0),a0
	move.l 10(a0),(a2)+
	move.l g_coreSegments+136,a0
	lea 0x4560(a0),a0
	jsr (a0)
	move.l d0,(a2)+
	move.l g_coreSegments+136,a0
	lea 0x463a(a0),a0
	jsr (a0)
	move.l d0,(a2)+
	move.l g_coreSegments+16,a0
	lea 0x3ae8(a0),a0
	jsr (a0)
	moveq #0,d0
	move.w -0x53a2(a5),d0
	move.l d0,(a2)+
	move.l g_coreSegments+16,a0
	lea 0x3b76(a0),a0
	jsr (a0)
	moveq #0,d0
	move.w -0x53a2(a5),d0
	move.l d0,(a2)+
	move.b #0x44,g_uiProbeReplyRaw
	move.w g_uiProbeAboutID,-(sp)
	move.l g_coreSegments+136,a0
	lea 0x245e(a0),a0
	jsr (a0)
	addq.l #2,sp
	move.l d0,(a2)+
	move.b #68,g_uiProbeReplyRaw
	move.l g_uiText,-(sp)
	move.w #401,-(sp)
	move.l g_coreSegments+136,a0
	lea 0x6d42(a0),a0
	jsr (a0)
	addq.l #6,sp
	move.l d0,(a2)+
	move.b #68,g_uiProbeReplyRaw
	move.l g_uiText,-(sp)
	move.w #404,-(sp)
	move.l g_coreSegments+136,a0
	lea 0x6d42(a0),a0
	jsr (a0)
	addq.l #6,sp
	move.l d0,(a2)+
	move.b #68,g_uiProbeReplyRaw
	move.l g_uiText,-(sp)
	move.w #405,-(sp)
	move.l g_coreSegments+136,a0
	lea 0x6d42(a0),a0
	jsr (a0)
	addq.l #6,sp
	move.l d0,(a2)+
	move.b #69,g_uiProbeReplyRaw
	move.l g_uiText,-(sp)
	move.w #405,-(sp)
	move.l g_coreSegments+136,a0
	lea 0x6d42(a0),a0
	jsr (a0)
	addq.l #6,sp
	move.l d0,(a2)+
	move.b #68,g_uiProbeReplyRaw
	move.l g_uiText,-(sp)
	move.w #406,-(sp)
	move.l g_coreSegments+136,a0
	lea 0x6d42(a0),a0
	jsr (a0)
	addq.l #6,sp
	move.l d0,(a2)+
	move.b #69,g_uiProbeReplyRaw
	move.l g_uiText,-(sp)
	move.w #406,-(sp)
	move.l g_coreSegments+136,a0
	lea 0x6d42(a0),a0
	jsr (a0)
	addq.l #6,sp
	move.l d0,(a2)+
	subq.l #4,sp
	move.w #'e',-(sp)
	.word 0xa93e
	move.l (sp)+,(a2)+
	subq.l #4,sp
	move.w #'o',-(sp)
	.word 0xa93e
	move.l (sp)+,(a2)+
	subq.l #4,sp
	move.w #'r',-(sp)
	.word 0xa93e
	move.l (sp)+,(a2)+
	subq.l #4,sp
	move.w #'p',-(sp)
	.word 0xa93e
	move.l (sp)+,(a2)+
	subq.l #4,sp
	move.w #'i',-(sp)
	.word 0xa93e
	move.l (sp)+,(a2)+
	subq.l #4,sp
	move.w #400,-(sp)
	clr.l -(sp)
	move.l #-1,-(sp)
	.word 0xaa46
	move.l (sp)+,g_uiWindow
	move.l g_uiWindow,-(sp)
	.word 0xa915
	move.l g_uiWindow,-(sp)
	.word 0xa91f
	subq.l #4,sp
	.word 0xa924
	move.l (sp)+,(a2)+
	move.l g_uiWindow,-(sp)
	.word 0xa916
	subq.l #4,sp
	.word 0xa924
	move.l (sp)+,(a2)+
	move.l g_uiWindow,-(sp)
	.word 0xa915
	subq.l #4,sp
	.word 0xa924
	move.l (sp)+,(a2)+
	rts
pop2_ui_fixture_end:

	.text
	.even
	.globl pop2_timer_callback
pop2_timer_callback:
	movem.l d0-d7/a0-a6,-(sp)
	move.w ccr,-(sp)
2:
	| Reuse the donor ports' user-mode callback drain loop. Each callback starts
	| with the interrupted register image; queued passes cannot nest callbacks.
	movem.l 2(sp),d0-d7/a0-a6
	move.l g_timerCallbackTask,a1
	move.l g_timerCallbackTask,a0
	move.l g_timerCallbackCode,-(sp)
	pea 1f
	move.l 4(sp),-(sp)
	rts
1:
	addq.l #4,sp
	jsr pop2TimerCallbackComplete
	tst.l d0
	bmi pop2_mac_exit
	bne 2b
	move.w (sp)+,ccr
	movem.l (sp)+,d0-d7/a0-a6
	rts
	.globl pop2_audio_callback
pop2_audio_callback:
	movem.l d0-d7/a0-a6,-(sp)
	move.w ccr,-(sp)
	move.l g_audioCallbackContext,-(sp)
	move.l g_audioCallbackCode,a0
	jsr (a0)
	addq.l #4,sp
	move.w (sp)+,ccr
	movem.l (sp)+,d0-d7/a0-a6
	rts
	.globl pop2_audio_lookup
pop2_audio_lookup:
	clr.l -(sp)
	move.l g_audioLookupType,-(sp)
	move.w #11,-(sp)
	.word 0xa9a0
	move.l (sp)+,a0
	move.l (a0),g_audioEntry
	move.l a0,-(sp)
	.word 0xa992
	rts
	.globl pop2_audio_invoke
pop2_audio_invoke:
	move.l g_audioArgument,-(sp)
	move.l g_audioSelector,-(sp)
	move.l g_audioEntry,a0
	movem.l d2-d7/a0-a6,g_audioBefore
	jsr (a0)
	movem.l d2-d7/a0-a6,g_audioAfter
	addq.l #8,sp
	move.l d0,g_audioResult
	rts
	.globl pop2_audio_test_callback
pop2_audio_test_callback:
	move.l 4(sp),a0
	.ifdef POP2_AUDIO_ORDER_PROBE
	move.w #7,24(a0)
	addq.l #1,40(a0)
	.else
	addq.l #1,(a0)
	.endif
	addq.l #1,g_audioCallbacks
	move.l #0x12345678,d0
	move.l #0x76543210,d1
	move.l d0,a0
	move.l d1,a1
	rts
	.globl pop2_audio_fixture_end
pop2_audio_fixture_end:

	.ifdef POP2_INTEGRATION_PROBE
	.section .text.integration_probe,"ax"
	.even
	.globl pop2_integration_seed
pop2_integration_seed:
	movem.l d2-d7/a2-a6,-(sp)
	move.l 48(sp),a0
	move.l 52(sp),a5
	move.w #0x1234,-(sp)
	jsr (a0)
	addq.l #2,sp
	movem.l (sp)+,d2-d7/a2-a6
	rts
	.globl pop2_integration_title
pop2_integration_title:
	movem.l d0-d7/a0-a6,-(sp)
	move.w ccr,-(sp)
	pea 62(sp)
	jsr pop2IntegrationTitleHook
	addq.l #4,sp
	move.w (sp)+,ccr
	movem.l (sp)+,d0-d7/a0-a6
	move.l g_integrationTitleTarget,-(sp)
	rts

	.globl pop2_integration_opening
pop2_integration_opening:
	movem.l d0-d7/a0-a6,-(sp)
	move.w ccr,-(sp)
	jsr pop2IntegrationOpeningHook
	move.w (sp)+,ccr
	movem.l (sp)+,d0-d7/a0-a6
	move.l g_integrationOpeningReturn,-(sp)
	rts
	.globl pop2_integration_room
pop2_integration_room:
	movem.l d0-d7/a0-a6,-(sp)
	move.w ccr,-(sp)
	jsr pop2IntegrationRoomHook
	move.w (sp)+,ccr
	movem.l (sp)+,d0-d7/a0-a6
	move.l g_integrationRoomReturn,-(sp)
	rts
	.ifdef POP2_GAMEPLAY_ROUTE_PROBE
	.globl pop2_gameplay_route_hall
pop2_gameplay_route_hall:
	movem.l d0-d7/a0-a6,-(sp)
	move.w ccr,-(sp)
	jsr pop2GameplayRouteHall
	move.w (sp)+,ccr
	movem.l (sp)+,d0-d7/a0-a6
	move.l g_routeHallReturn,-(sp)
	rts
	.globl pop2_gameplay_route_ending
pop2_gameplay_route_ending:
	movem.l d0-d7/a0-a6,-(sp)
	move.w ccr,-(sp)
	jsr pop2GameplayRouteEnding
	move.w (sp)+,ccr
	movem.l (sp)+,d0-d7/a0-a6
	move.l g_routeEndingReturn,-(sp)
	rts
	.globl pop2_gameplay_route_frame
pop2_gameplay_route_frame:
	movem.l d0-d7/a0-a6,-(sp)
	move.w ccr,-(sp)
	move.l d0,-(sp)
	jsr pop2GameplayRouteFrame
	addq.l #4,sp
	move.w (sp)+,ccr
	movem.l (sp)+,d0-d7/a0-a6
	move.l g_routeReturn,-(sp)
	rts
	.endif
	.ifdef POP2_GAMEPLAY_SAVE_PROBE
	.globl pop2_gameplay_saved
pop2_gameplay_saved:
	movem.l d0-d7/a0-a6,-(sp)
	move.w ccr,-(sp)
	move.l d0,-(sp)
	jsr pop2GameplaySaved
	addq.l #4,sp
	move.w (sp)+,ccr
	movem.l (sp)+,d0-d7/a0-a6
	move.l g_gameplaySaveReturn,-(sp)
	rts
	.globl pop2_gameplay_loaded
pop2_gameplay_loaded:
	movem.l d0-d7/a0-a6,-(sp)
	move.w ccr,-(sp)
	move.l d0,-(sp)
	jsr pop2GameplayLoaded
	addq.l #4,sp
	move.w (sp)+,ccr
	movem.l (sp)+,d0-d7/a0-a6
	move.l g_gameplayLoadReturn,-(sp)
	rts
	.endif
	.globl pop2_integration_death
pop2_integration_death:
	movem.l d0-d7/a0-a6,-(sp)
	move.w ccr,-(sp)
	pea 62(sp)
	jsr pop2IntegrationDeathHook
	addq.l #4,sp
	move.w (sp)+,ccr
	movem.l (sp)+,d0-d7/a0-a6
	move.l g_integrationDeathTarget,-(sp)
	rts
	.globl pop2_integration_death_return
pop2_integration_death_return:
	movem.l d0-d7/a0-a6,-(sp)
	move.w ccr,-(sp)
	jsr pop2IntegrationDeathReturned
	move.w (sp)+,ccr
	movem.l (sp)+,d0-d7/a0-a6
	move.l g_integrationDeathReturn,-(sp)
	rts

	.endif

	.ifdef POP2_TIMING_VBL_PROBE
	.text
	.globl pop2_timing_vbl_fixture,pop2_timing_vbl_callback,pop2_timing_vbl_end
| Authored counterpart of mac_timing.lua: four countdowns, thirty ticks,
| three callbacks then automatic removal, followed by explicit VRemove.
| The callback deliberately damages registers to check deferred dispatch ABI.
pop2_timing_vbl_fixture:
	movem.l d2-d7/a2-a6,-(sp)
	move.l #0x13579bdf,d2
	move.l #0x2468ace0,a3
	move.l g_timingVblRecords,a2
	moveq #3,d7
1:
	move.l a2,a0
	.word 0xa033
	move.w d0,18(a2)
	lea 32(a2),a2
	dbra d7,1b
	subq.l #4,sp
	.word 0xa975
	move.l (sp)+,d6
2:
	subq.l #4,sp
	moveq #0,d0
	.word 0xa975
	bne 8f
	cmp.l #0x13579bdf,d2
	bne 8f
	cmpa.l #0x2468ace0,a3
	bne 8f
	move.l (sp)+,d0
	sub.l d6,d0
	cmp.l #30,d0
	blo 2b
	move.l d0,g_timingVblElapsed
	move.l g_timingVblRecords,a2
	moveq #3,d7
3:
	move.w 10(a2),24(a2)
	move.l a2,a0
	.word 0xa034
	move.w d0,26(a2)
	lea 32(a2),a2
	dbra d7,3b
| Fifth task: install dormant, then defer all original callbacks for six
| PAL fields. One safe point must drain separate virtual VBL passes.
	move.l a2,a0
	.word 0xa033
	move.w d0,18(a2)
	move.w #1,10(a2)
	move.l g_scaffoldFields,d5
5:
	move.l g_scaffoldFields,d0
	sub.l d5,d0
	cmp.l #6,d0
	blo 5b
	move.l d0,g_timingVblDeferredFields
	subq.l #4,sp
	moveq #0,d0
	.word 0xa975
	bne 8f
	cmp.l #0x13579bdf,d2
	bne 8f
	cmpa.l #0x2468ace0,a3
	bne 8f
	addq.l #4,sp
	move.w 10(a2),24(a2)
	move.l a2,a0
	.word 0xa034
	move.w d0,26(a2)
	moveq #0,d0
	.word 0xaffb
8:
	moveq #2,d0
	.word 0xaffb
pop2_timing_vbl_callback:
	addq.w #1,16(a1)
	subq.l #4,sp
	.word 0xa975
	move.l (sp)+,20(a1)
	cmp.w #1,16(a1)
	bne 6f
	move.l 20(a1),28(a1)
6:
	cmp.w #3,16(a1)
	bhs 4f
	move.w #1,10(a1)
4:
	move.l #0xdeadbeef,d2
	move.l #0xfeedface,a3
	rts
pop2_timing_vbl_end:
	.endif

	.ifdef POP2_TIMING_TM_PROBE
	.text
	.globl pop2_timing_tm_fixture,pop2_timing_tm_callback,pop2_timing_tm_end
	.macro timing_tm_case offset,delay,second=0,cancel=0
	move.l g_timingTmRecords,a2
	lea \offset(a2),a2
	move.l a2,a0
	.word 0xa058
	move.w d0,26(a2)
	subq.l #4,sp
	.word 0xa975
	move.l (sp)+,32(a2)
	move.l #\delay,d0
	move.l a2,a0
	.word 0xa05a
	move.w d0,28(a2)
	.if \second
	move.l #\second,d0
	move.l a2,a0
	.word 0xa05a
	move.w d0,56(a2)
	.endif
	.if \cancel
	move.l a2,a0
	.word 0xa059
	move.w d0,30(a2)
	.endif
	move.l 32(a2),d6
1:
	subq.l #4,sp
	moveq #0,d0
	.word 0xa975
	bne timing_tm_abi_failure
	cmp.l #0x13579bdf,d2
	bne timing_tm_abi_failure
	cmpa.l #0x2468ace0,a3
	bne timing_tm_abi_failure
	move.l (sp)+,d0
	sub.l d6,d0
	cmp.l #20,d0
	blo 1b
	move.l 10(a2),40(a2)
	move.w 4(a2),44(a2)
	move.l a2,a0
	.word 0xa059
	move.w d0,58(a2)
	.endm
| Authored counterpart of mac_time_manager.lua. All waits use services so
| original callbacks are dispatched at user-mode safe points.
pop2_timing_tm_fixture:
	movem.l d2-d7/a2-a6,-(sp)
	move.l #0x13579bdf,d2
	move.l #0x2468ace0,a3
	timing_tm_case 0,40
	timing_tm_case 64,-40000
	timing_tm_case 128,80,1
	timing_tm_case 192,20,100
	timing_tm_case 256,200,0,1
	timing_tm_case 320,0
	timing_tm_case 384,20
	moveq #0,d0
	.word 0xaffa
 timing_tm_abi_failure:
	moveq #2,d0
	.word 0xaffa
pop2_timing_tm_callback:
	addq.w #1,24(a1)
	subq.l #4,sp
	.word 0xa975
	move.l (sp)+,36(a1)
	move.w 4(a1),52(a1)
	cmp.w #1,24(a1)
	bne 2f
	move.l 36(a1),48(a1)
2:
	tst.w 60(a1)
	beq 3f
	cmp.w #3,24(a1)
	bhs 3f
	move.l a1,a0
	moveq #20,d0
	.word 0xa05a
3:
	move.l #0xdeadbeef,d2
	move.l #0xfeedface,a3
	rts
pop2_timing_tm_end:
	.endif

	.text
	.include "../src/platform/amiga/RectProbe.inc"
