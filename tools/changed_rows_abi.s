	.text
	.even
	.globl rowsFixtureCall
rowsFixtureCall:
	movem.l d2-d7/a2-a6,-(sp)
	move.l 76(sp),a0
	move.l a0,-(sp)
	move.l 76(sp),-(sp)
	move.l 76(sp),-(sp)
	move.l 76(sp),-(sp)
	move.l 76(sp),-(sp)
	move.l 76(sp),-(sp)
	move.l 76(sp),-(sp)
	move.l 76(sp),-(sp)
	move.l #0x11220000,d2
	move.l #0x11220101,d3
	move.l #0x11220202,d4
	move.l #0x11220303,d5
	move.l #0x11220404,d6
	move.l #0x11220505,d7
	move.l #0x11220606,a2
	move.l #0x11220707,a3
	move.l #0x11220808,a4
	move.l #0x11220909,a5
	move.l #0x11220a0a,a6
	jsr pop2CopyBitsChangedRows
	lea 28(sp),sp
	move.l (sp)+,a0
	movem.l d2-d7/a2-a6,(a0)
	movem.l (sp)+,d2-d7/a2-a6
	rts
