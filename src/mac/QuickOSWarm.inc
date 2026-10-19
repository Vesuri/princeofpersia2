| C ABI: (saved D0-D7/A0-A6 image, exception frame) -> success boolean.
| Caller has validated the trap owner, stack and pending-service certificate.
| Only D0/D1/A0/A1 are clobbered. Misses tail-call the checked C body without
| changing guest state. Pinned hints contain full slot identity + live flags
| pointer, not cached allocation state. Reset must clear hints before reuse.
	.macro mac_handle_warm entry,slow,hints,error,memerror,mask,lock
	.text
	.even
	.globl \entry
\entry:
	move.l 4(sp),a0
	move.l 32(a0),d0
	move.l d0,d1
	lsr.l #2,d1
	andi.l #\mask,d1
	move.l \hints,a1
	lea (a1,d1.l*8),a1
	cmp.l (a1),d0
	bne.s .Lmiss\@
	move.l 4(a1),d0
	beq.s .Lmiss\@
	move.l d0,a1
	btst #0,(a1)
	beq.s .Lmiss\@
	moveq #0,d0
	move.b (a1),d0
	andi.w #0xe0,d0
	.if \lock
	ori.b #0x81,d0
	move.b d0,(a1)
	moveq #0,d0
	.endif
	move.l d0,(a0)
	move.l \error,a1
	clr.w (a1)
	move.l \memerror,a1
	clr.w (a1)
	move.l 8(sp),a1
	andi.w #0xfff0,(a1)
	tst.w d0
	bne.s .Lresult\@
	ori.w #4,(a1)
.Lresult\@:
	moveq #1,d0
	rts
.Lmiss\@:
	jmp \slow
	.endm

| SwapMMUMode accepts only low-byte modes 0/1, returns the previous mode,
| and republishes the existing heap error (unlike the handle query bodies).
	.macro mac_mmu_warm entry,slow,mode,shadow,error,memerror
	.text
	.even
	.globl \entry
\entry:
	move.l 4(sp),a0
	moveq #0,d1
	move.b 3(a0),d1
	cmpi.b #1,d1
	bhi.s .Lmiss\@
	move.l \mode,a1
	moveq #0,d0
	move.b (a1),d0
	move.b d1,(a1)
	move.l \shadow,a1
	move.b d1,(a1)
	move.l d0,(a0)
	move.l \error,a1
	move.w (a1),d1
	move.l \memerror,a1
	move.w d1,(a1)
	move.l 8(sp),a1
	andi.w #0xfff0,(a1)
	tst.w d0
	bne.s .Lresult\@
	ori.w #4,(a1)
.Lresult\@:
	moveq #1,d0
	rts
.Lmiss\@:
	jmp \slow
	.endm
