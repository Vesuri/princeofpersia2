; WHDLoad/Kickstart 3.1 launcher for the Amiga port. Cross-assembled with vasm.
; The executable uses Exec/graphics/DOS; kickfs supplies ordinary file access.

        INCLUDE whdload.i
        INCLUDE whdmacros.i

CHIPMEMSIZE = $200000
FASTMEMSIZE = $600000 ; validation pending; host OS/PRELOAD need additional RAM
NUMDRIVES = 0
REASONSIZE = 160                ; game stop reason, including its NUL
GVF_LOCAL_ONLY = $200
WPDRIVES = 0
BLACKSCREEN
BOOTDOS
INITAGA                         ; publish AGA through graphics.library
CACHE
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
slv_name dc.b "Prince of Persia 2",0
slv_copy dc.b "1993, 1994 Jordan Mechner / Broderbund",0
slv_info dc.b "Amiga port by Vesuri",10
        dc.b "Version 0.92 (31.10.2026)",10
        dc.b "F10 quits",0
slv_config dc.b 0
        dc.b "$VER: PrinceOfPersia2.slave 0.92 (31.10.2026)",0
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
        IFD TEST
        lea (_diagnostic,pc),a0
        move.l a0,d1
        move.l #1006,d2
        jsr (_LVOOpen,a6)
        move.l d0,d1
        jsr (_LVOSelectOutput,a6)
        ENDC
        lea (_program,pc),a0
        move.l a0,d1
        jsr (_LVOLoadSeg,a6)
        move.l d0,d7
        beq .readerror
        IFD TEST
        lea (_loadmark,pc),a0
        bsr _mark
        ENDC
        bsr _patch_binding
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
        ; A failing game leaves its stop reason in a local variable.
        tst.l d6
        beq .noreason
        lea (_reasonvar,pc),a0
        move.l a0,d1
        lea (_reason,pc),a0
        move.l a0,d2
        move.l #REASONSIZE,d3
        move.l #GVF_LOCAL_ONLY,d4
        jsr (_LVOGetVar,a6)
.noreason
        IFD TEST
        tst.l d6
        bne .gameerror
        ENDC
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
        lea (_failed,pc),a0
        tst.b (_reason,pc)
        beq .message
        lea (_stopped,pc),a0
.message
        move.l a0,-(sp)
        pea TDREASON_FAILMSG
.abort
        move.l (_resload,pc),a2
        jmp (resload_Abort,a2)
_failed dc.b "Prince of Persia 2 could not start. See the game diagnostics.",0
_reasonvar dc.b "Pop2Reason",0
; The reason buffer completes the stopped message.
_stopped dc.b "Prince of Persia 2 stopped: "
_reason ds.b REASONSIZE
        dc.b 0
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
_diagnostic dc.b "startup.log",0
_marker dc.b "PASS"
_earlymark dc.b "test-early",0
_bootmark dc.b "test-bootdos",0
_loadmark dc.b "test-loaded",0
_exitmark dc.b "test-returned",0
        EVEN
        ENDC

BINDING_MAGIC = $504f5032       ; POP2
        INCLUDE PatchBinding.s
_binding_missing
        pea (_binding_error,pc)
        pea TDREASON_FAILMSG
        jmp (resload_Abort,a2)
_binding_error dc.b "Prince of Persia 2 WHDLoad binding missing or invalid.",0
        EVEN
