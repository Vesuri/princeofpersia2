# Kalms C2P

The release uses AmigaXDev's shared `KalmsC2PRect.s` adapter and the unchanged
public-domain `normal/c2p1x1_8_c5_030_2.s` from Kalmalyzer/kalms-c2p commit
`d8ecf79a3325615305dd800ae7704b518e0d9dda`. `manifest/kalms-c2p.sha256` pins
both files before assembly. `manifest/references.tsv` identifies the shared
revision. The historical generic kernel remains as the earlier source reference.

`DisplayC2P.s` supplies this port's 640-byte source stride, 512-byte planar row
and 64-byte plane spacing. Existing dirty bands and frame-end publication are
unchanged. The ordinary initializer is used; the upstream self-modifying entry
is never called. NativeDisplay reserves four private bytes after its physical
screen for the unused source prefetch; the shared adapter also pads its scratch
buffer. The public PixMap remains 640×480. `NDK_ASM` in `amiga/env.sh` supplies
the upstream Exec LVO include path.

See `docs/performance.md` for the fixed-68020 measurements and complete-plane,
source/guard-memory and saved-register validation. A routine's 030 name is not
a performance claim for another CPU or port.
