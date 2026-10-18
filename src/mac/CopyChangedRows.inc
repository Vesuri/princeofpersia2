| 68020+ C ABI body: src,dst,width,height,srcStride,dstStride,RowSpan*.
| Distinct valid buffers; width 0..640, height 0..480, strides >= width.
| RowSpan is two uint16 values, [left,right), conservative by at most 3 bytes
| at each changed edge. Empty rows have left==right; all other bytes untouched.
| Preserves d2-d7/a2-a6 and SP. d0-d1/a0-a1/CCR are scratch as in the C ABI.
| No original data or game-specific state; caller supplies the function label.
 .macro copy_changed_rows
 movem.l d2-d7/a2-a6,-(sp)
 move.l 48(sp),a2
 move.l 52(sp),a3
 move.l 56(sp),d2
 move.l 60(sp),d3
 move.l 64(sp),d4
 move.l 68(sp),d5
 move.l 72(sp),a4
 tst.l d3
 beq .Lchanged_done_\@
 subq.w #1,d3
.Lchanged_row_\@:
 move.l a2,a0
 move.l a3,a1
 moveq #0,d6
 move.l d2,d7
 move.l d2,d0
 lsr.l #2,d0
 beq .Lchanged_left_bytes_\@
 subq.w #1,d0
.Lchanged_left_longs_\@:
 cmpm.l (a0)+,(a1)+
 bne .Lchanged_right_start_\@
 addq.w #4,d6
 dbra d0,.Lchanged_left_longs_\@
.Lchanged_left_bytes_\@:
 cmp.w d2,d6
 beq .Lchanged_record_\@
 cmpm.b (a0)+,(a1)+
 bne .Lchanged_right_start_\@
 addq.w #1,d6
 bra .Lchanged_left_bytes_\@
.Lchanged_right_start_\@:
 move.l a2,a0
 move.l a3,a1
 adda.l d2,a0
 adda.l d2,a1
 move.l d7,d1
 sub.w d6,d1
 lsr.l #2,d1
 beq .Lchanged_right_bytes_\@
 subq.w #1,d1
.Lchanged_right_longs_\@:
 move.l -(a0),d0
 cmp.l -(a1),d0
 bne .Lchanged_copy_\@
 subq.w #4,d7
 dbra d1,.Lchanged_right_longs_\@
.Lchanged_right_bytes_\@:
 cmp.w d6,d7
 beq .Lchanged_record_\@
 move.b -(a0),d0
 cmp.b -(a1),d0
 bne .Lchanged_copy_\@
 subq.w #1,d7
 bra .Lchanged_right_bytes_\@
.Lchanged_copy_\@:
 move.l a2,a0
 move.l a3,a1
 adda.w d6,a0
 adda.w d6,a1
 move.l d7,d0
 sub.w d6,d0
 move.l d0,d1
 lsr.l #4,d1
 beq .Lchanged_copy_longs_\@
 subq.w #1,d1
.Lchanged_copy_blocks_\@:
 move.l (a0)+,(a1)+
 move.l (a0)+,(a1)+
 move.l (a0)+,(a1)+
 move.l (a0)+,(a1)+
 dbra d1,.Lchanged_copy_blocks_\@
.Lchanged_copy_longs_\@:
 move.l d0,d1
 andi.w #15,d1
 lsr.w #2,d1
 beq .Lchanged_copy_bytes_\@
 subq.w #1,d1
.Lchanged_copy_long_\@:
 move.l (a0)+,(a1)+
 dbra d1,.Lchanged_copy_long_\@
.Lchanged_copy_bytes_\@:
 andi.w #3,d0
 beq .Lchanged_record_\@
 subq.w #1,d0
.Lchanged_copy_byte_\@:
 move.b (a0)+,(a1)+
 dbra d0,.Lchanged_copy_byte_\@
.Lchanged_record_\@:
 move.w d6,(a4)+
 move.w d7,(a4)+
 adda.l d4,a2
 adda.l d5,a3
 dbra d3,.Lchanged_row_\@
.Lchanged_done_\@:
 movem.l (sp)+,d2-d7/a2-a6
 rts
 .endm
