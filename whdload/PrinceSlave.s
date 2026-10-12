; WHDLoad/Kickstart 3.1 launcher for the Amiga port. Cross-assembled with vasm.
; The executable uses Exec/graphics/DOS; kickfs supplies ordinary file access.

        INCLUDE whdload.i
        INCLUDE whdmacros.i

CHIPMEMSIZE = $200000
FASTMEMSIZE = $200000 ; scaffold only; final requirement remains unmeasured
NUMDRIVES = 0
WPDRIVES = 0
BLACKSCREEN
BOOTDOS
CACHECHIP
HDINIT
SEGTRACKER
NO68020                         ; portable kickemu patches; game still needs 020
; Do not use kick31.s STACKSIZE: its A600 patch at $2305c overwrites
; the MOVE.L opcode (the immediate starts at $2305e).
        IFD TEST
BOOTEARLY
DEBUG
        ENDC

        IFD SNOOPFS
slv_Version = 18
        ELSE
slv_Version = 17
        ENDC
slv_Flags = WHDLF_NoError|WHDLF_EmulLineA|WHDLF_Req68020|WHDLF_ReqAGA
slv_keyexit = $59                 ; F10
        INCLUDE whdload/kick31.s

slv_CurrentDir dc.b 0
slv_name dc.b "Prince of Persia 2 scaffold",0
slv_copy dc.b "Port scaffold",0
slv_info dc.b "Amiga port by Vesuri",10
        dc.b "Version 0.1 (12.10.2026)",10
        dc.b "F10 quits",0
slv_config dc.b 0
        dc.b "$VER: PrinceOfPersia2.slave 0.1 (12.10.2026)",0
_program dc.b "PrinceOfPersia2",0
_args dc.b 10
        EVEN

_bootdos
        move.l (_resload,pc),a2
        IFD BOOTONLY
        pea TDREASON_OK
        jmp (resload_Abort,a2)
        ENDC
        IFD TEST
        lea (_bootmark,pc),a0
        bsr _mark
        ENDC
        lea (_dosname,pc),a1
        move.l 4.w,a6
        jsr (_LVOOldOpenLibrary,a6)
        move.l d0,a6
        tst.l d0
        beq .oserror
        lea (_program,pc),a0
        move.l a0,d1
        jsr (_LVOLoadSeg,a6)
        move.l d0,d7
        beq .readerror
        IFD TEST
        lea (_loadmark,pc),a0
        bsr _mark
        ENDC
        jsr (resload_FlushCache,a2)
        ; Establish PROGDIR as a Shell would. Calling a LoadSeg entry alone
        ; does not set pr_HomeDir, which the game's resource loader uses.
        lea (_current,pc),a0
        move.l a0,d1
        moveq #-2,d2              ; ACCESS_READ, lock the actual current drawer
        jsr (_LVOLock,a6)
        tst.l d0
        beq .readerror
        move.l d0,d1
        jsr (_LVOSetProgramDir,a6)
        lea (_oldhome,pc),a0
        move.l d0,(a0)
        IFD LOADONLY
        pea TDREASON_OK
        jmp (resload_Abort,a2)
        ENDC
        ; Use Exec's public API instead of modifying Kickstart's CLI code.
        move.l a6,-(sp)
        move.l 4.w,a6
        move.l #4096,d0
        moveq #0,d1
        jsr (_LVOAllocMem,a6)
        tst.l d0
        beq .oserror
        lea (_stackmem,pc),a0
        move.l d0,(a0)
        lea (_stack,pc),a0
        move.l d0,(a0)
        add.l #4096,d0
        move.l d0,(4,a0)
        move.l d0,(8,a0)
        jsr (_LVOStackSwap,a6)
        move.l d7,a1
        add.l a1,a1
        add.l a1,a1
        moveq #1,d0
        lea (_args,pc),a0
        jsr (4,a1)
        move.l d0,d6
        move.l 4.w,a6
        lea (_stack,pc),a0
        jsr (_LVOStackSwap,a6)
        move.l (_stackmem,pc),a1
        move.l #4096,d0
        jsr (_LVOFreeMem,a6)
        move.l (sp)+,a6
        move.l (_oldhome,pc),d1
        jsr (_LVOSetProgramDir,a6)
        move.l d0,d1
        jsr (_LVOUnLock,a6)
        move.l d7,d1
        jsr (_LVOUnLoadSeg,a6)
        move.l a6,a1
        move.l 4.w,a6
        jsr (_LVOCloseLibrary,a6)
        tst.l d6
        bne .gameerror
        IFD TEST
        lea (_exitmark,pc),a0
        bsr _mark
        ENDC
        pea TDREASON_OK
        bra .abort
.readerror
        jsr (_LVOIoErr,a6)
        pea (_program,pc)
        move.l d0,-(sp)
        pea TDREASON_DOSREAD
        bra .abort
.oserror
        clr.l -(sp)
        clr.l -(sp)
        pea TDREASON_OSEMUFAIL
        bra .abort
.gameerror
        pea (_failed,pc)
        pea TDREASON_FAILMSG
.abort
        move.l (_resload,pc),a2
        jmp (resload_Abort,a2)
_failed dc.b "Prince of Persia 2 scaffold could not start. See the scaffold diagnostics.",0
_current dc.b 0
        EVEN
_stackmem dc.l 0
_stack dc.l 0,0,0
_oldhome dc.l 0
        IFD TEST
_bootearly
        move.l (_resload,pc),a2
        lea (_earlymark,pc),a0
        bra _mark
_mark
        movem.l d0-d1/a0-a1,-(sp)
        lea (_marker,pc),a1
        moveq #4,d0
        jsr (resload_SaveFile,a2)
        movem.l (sp)+,d0-d1/a0-a1
        rts
_marker dc.b "PASS"
_earlymark dc.b "test-early",0
_bootmark dc.b "test-bootdos",0
_loadmark dc.b "test-loaded",0
_exitmark dc.b "test-returned",0
        EVEN
        ENDC
