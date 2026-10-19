.section .rodata.version,"aR",@progbits
.global pop2Version
pop2Version: .asciz "$VER: Prince of Persia 2 0.90 (19.10.2026)"

.section .data.whdload,"awR",@progbits
.balign 4
.global pop2WHDLoadBinding
pop2WHDLoadBinding:
.long 0x504f5032,0x57484452,0x00020000,0
