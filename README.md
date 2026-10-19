# Prince of Persia 2 — Amiga

An unofficial Amiga port of **Prince of Persia 2: The Shadow and the Flame**,
based on the Macintosh version by Jordan Mechner / Brøderbund.

The original 68k game code runs natively with Amiga implementations of the
Macintosh services it uses. The port keeps the original 512×384 graphics on a
PAL hires interlaced display, with music and sound effects through Paula.
Hard disk installation, saved games and quitting to Workbench are supported.
Joystick input is not supported yet.

**Version 0.90 (19.10.2026) is a release candidate.** The game is playable and
tested in emulation; real-Amiga hardware qualification is still pending.

## Requirements

- PAL **AGA Amiga**, **68030 or better**.
- **2 MB Chip RAM and at least 8 MB Fast RAM**. Allow additional memory for
  WHDLoad and PRELOAD.
- WHDLoad 18.0 or newer and Installer 43 or newer.
- An A1200 Kickstart 3.1 image and its matching RTB file in `DEVS:Kickstarts`,
  named `kick40068.A1200` and `kick40068.A1200.RTB`.
- At least **40 MB free disk space**, plus space to preserve an existing
  installation.

Supply your own licensed Kickstart image. The matching RTB is available in
`util/boot/skick346.lha` on Aminet.

## Installation

1. Unpack `PrinceOfPersia2-0.90.lha` and open the **PoP2 Install** drawer.
2. Obtain `Prince_Of_Persia2.SIT` from
   [Macintosh Repository](https://www.macintoshrepository.org/download.php?id=4673).
   Leave the downloaded file compressed.
3. Run **Install**, select the destination parent drawer, and select the SIT
   archive when asked. Extraction and verification can take several minutes.
4. Start the installed **PrinceOfPersia2** icon.

When updating, **Use existing** keeps verified installed game data;
**Reinstall** extracts it again. Both preserve saved games and preferences and
keep a backup of the previous installation. The program, slave, icon and ReadMe
are always updated.

PRELOAD is enabled by default. Disable that Tooltype if there is insufficient
memory to cache the files. No original game data, Kickstart, Workbench or
WHDLoad binaries are included in the package.

See [the packaged ReadMe](release/ReadMe) for the installation instructions.

## Controls

| Key | Action |
| --- | --- |
| Space | Start play |
| Cursor keys | Move |
| Shift | Careful movement and grabbing |
| Ctrl | Sword actions |
| Amiga-N / Amiga-E | New game / end game |
| Amiga-O / Amiga-S | Load / save game |
| Amiga-P | Pause |
| Amiga-T / Amiga-M | Toggle sound / music |
| Amiga-R | Restart |
| Amiga-H | Hall of fame |
| Amiga-Q | Quit normally |
| F10 | Quit WHDLoad |

## Building and development

Source builds use AmigaXDev's toolchain, including `m68k-amiga-elf-gcc`,
`elf2hunk` and vasm. With the toolchain and required local inputs prepared,
`make dist` builds the installer archive without original game data.

See [development setup](docs/development.md) for build commands, tests and
emulator setup. [Release validation](docs/release.md) documents the tested
configurations and remaining hardware checks.

## Credits and licensing

Original game: Jordan Mechner / Brøderbund. Amiga port: Vesa Halttunen (Vesuri).
Amiga framework: dA JoRMaS. Chunky-to-planar conversion: Mikael Kalms.

This is an unofficial fan port, not affiliated with or endorsed by the original
creators or rights holders. The original game and its assets remain the
property of their respective copyright holders.

The standalone extraction helper uses MacPaw/XADMaster and is licensed under
LGPL-2.1-or-later. See [the helper's source and licensing notes](tools/install-data/README.md).
That license applies to the helper only, not to the original game.
