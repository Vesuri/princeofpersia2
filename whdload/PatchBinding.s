; Shared retained 16-byte ABI v2 scanner from Alone in the Dark.
; Input: D7 DOS segment list, A2 resload. Output: A0 binding start.
; Clobbers D0-D1/A0-A1. Consumer defines BINDING_MAGIC and _binding_missing.
; Layout: 8-byte magic, version word=2, switch epoch word=0, resload pointer=0.
_patch_binding
        move.l d7,d0
.seg    tst.l d0
        beq _binding_missing
        add.l d0,d0
        add.l d0,d0
        move.l d0,a0
        move.l (-4,a0),d1
        move.l (a0)+,d0
        sub.l #24,d1
        bmi .seg
        move.l a0,a1
        add.l d1,a1
.scan   cmpa.l a1,a0
        bhi .seg
        cmp.l #BINDING_MAGIC,(a0)+
        bne .scan
        cmp.l #$57484452,(a0)    ; WHDR
        bne .scan
        cmp.l #$00020000,(4,a0)
        bne .scan
        tst.l (8,a0)
        bne .scan
        move.l a2,(8,a0)
        subq.l #4,a0
        rts
