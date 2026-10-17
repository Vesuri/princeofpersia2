; C ABI adapter for Mikael Kalms' public-domain c2p1x1_8_c5_030_2.
; void amigaxdevKalmsC2PRect(const uint8_t *source, uint8_t *destination,
;                      uint32_t width, uint32_t rows);
; Caller supplies C2P_SOURCE_STRIDE, C2P_OUTPUT_STRIDE, C2P_PLANE_STRIDE
; and C2P_MAX_WIDTH. Planes are interleaved within each output row.
; Caller supplies a word-aligned destination, nonempty width divisible by 32
; (<=C2P_MAX_WIDTH) and positive rows. Source may be unaligned on 68020+.
; The kernel prefetches one unused longword past each source row: the caller
; must supply four readable bytes after each converted span. No SMC init is used.
        section code,code
        xdef amigaxdevKalmsC2PRect
amigaxdevKalmsC2PRect:
        movem.l d2-d3/a2-a3,-(sp)
        move.l  20(sp),a2
        move.l  24(sp),a3
        move.l  28(sp),d0
        move.l  32(sp),d2
        moveq   #1,d1
        moveq   #0,d3
        bsr     c2p1x1_8_c5_030_2_init
.row:
        move.l  a2,a0
        move.l  a3,a1
        bsr     c2p1x1_8_c5_030_2
        adda.w  #C2P_SOURCE_STRIDE,a2
        adda.w  #C2P_OUTPUT_STRIDE,a3
        subq.l  #1,d2
        bne.s   .row
        movem.l (sp)+,d2-d3/a2-a3
        rts
BPLX    equ C2P_MAX_WIDTH
BPLY    equ 1
BPLSIZE equ C2P_PLANE_STRIDE
        include "c2p1x1_8_c5_030_2.s"

; The final scratch read is prefetch only, but keep it inside allocated BSS.
        ds.l 1
