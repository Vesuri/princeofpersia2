; Port geometry for AmigaXDev's shared Kalms rectangle adapter.
; NativeDisplay reserves four private padding bytes after the physical screen
; for the unchanged kernel prefetch, including bottom-right viewports.
        section code,code
        xdef pop2KalmsC2PRect
pop2KalmsC2PRect:
C2P_SOURCE_STRIDE equ 640
C2P_OUTPUT_STRIDE equ 512
C2P_PLANE_STRIDE  equ 64
C2P_MAX_WIDTH     equ 512
        incdir "../src/platform/amiga/kalms/"
        include "KalmsC2PRect.s"
