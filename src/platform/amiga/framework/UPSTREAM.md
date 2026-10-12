# Framework provenance

Canonical dA JoRMaS framework from AmigaXDev commit
`cbec18036ea0ccda2fbdbff0e6f4eedb1463a63b`, unchanged, with neutral DAJ prefixes.
The copied-file/source-hash inventory is `manifest/scaffold-sources.tsv`.
Includes AmigaHardware, Bitmap, CopperList, Palette, Palette24Bit, Sprite, Util,
SASCCompat, compatibility headers and four assembler implementations.
No Production/Part orchestration. The Amiga build compiles all these modules
and partial-links them to expose dormant unresolved references.

Only the empty takeover/handback path is exercised by this scaffold. Display
geometry, palette publication and C2P are not Prince of Persia 2 validation yet.
Compare against the canonical `$AMIGA_FRAMEWORK` with the shared framework tool.
There are no local framework modifications.
