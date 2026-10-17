| DERIVED: replacement for CODE 17 + $53A4..$53C5 only.
| The original outer routine retains allocation, clipping, row stepping and
| header writes. Width is a positive multiple of four, at most $7FFC.
| Preserve the exact ordered longword overlap semantics, all registers and
| frame locals. A0 ends at the last destination long, D0 at width, CCR=Z.
	.include "src/mac/ForwardLongCopy.inc"
	.section .text.pop2SaveRectRow,"ax"
	.globl pop2SaveRectRow
pop2SaveRectRow:
	movem.l d1/a1,-(sp)
	movea.l -32(a6),a0
	movea.l -36(a6),a1
	move.l -8(a6),d1
	lsr.l #2,d1
	subq.l #1,d1
	forward_long_copy a0,a1,d1
	move.l a0,-32(a6)
	move.l a1,-36(a6)
	lea -4(a1),a0
	move.l -8(a6),d0
	move.l d0,-48(a6)
	movem.l (sp)+,d1/a1
	cmp.l -8(a6),d0
	rts
