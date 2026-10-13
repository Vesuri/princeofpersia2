# Prince of Persia 2

Macintosh-to-Amiga port following Vette! and Alone in the Dark. The owner has set the
target and approach in `docs/design.md`; work proceeds through `docs/open-work.md` in
order. Inspection, extraction, a verified Macintosh reference and a static/live map exist.
Core, file/resource, display, UI and audio service gates exist. The native startup
integration gate reaches the first playable room with matching reference pixels
and state and verified gameplay input. Earlier service fixtures use explicit
checkpoints and do not independently establish gameplay. Timing and later
validation remain open.

## Rules

- Never commit original or byte-derived material: game files, resource forks, ROMs,
  Kickstart, Workbench, WHDLoad, generated code containing original tables, disassembly,
  captures, traces, saves or emulator state. Keep originals and scratch in ignored
  `tmp/`; generated and reference directories are ignored too. Check
  `git status --ignored` before adding new files. Commit names and hashes only.
- The original Macintosh execution is the ground truth. Preserve original game
  decisions; implement the services it calls. Unknown calls, control flow, code writes,
  device accesses and interrupts remain named loud stops, never guessed successes.
- Correctness needs tests against the original and positive completion signals.
  A timeout or an unreached probe is not a pass; negative observations need controls.
- No performance claims without matched-workload measurements.
- Never kill emulators or debuggers by process name. Use this project's recorded PIDs
  and the shared `FSUAE_COMMON` helper. Never edit a running shell launcher.
- No personal paths or paths into other projects in tracked files. Name other projects
  by name. Shared paths use overridable variables from `amiga/env.sh`; `~/.local`
  and `~/.local/share/amiga` are the allowed conventions.
- Pin inputs by SHA-256 or commit in `manifest/` and verify before use. Preserve
  original inputs. Setup never deletes: move differing files to `.replaced/`.
- Before implementing or fixing a service, inspect Vette! and Alone in the Dark
  first. Reuse existing implementations and share reusable code between the ports
  through the shared workflow; do not create independent copies of common logic.
- Shared files edited in place must be brought back through AmigaXDev's
  `make pull-shared`, never overwritten. Do not change other projects with dirty trees.
- Shell scripts must work with macOS `/bin/bash` 3.2.
- `docs/open-work.md` is the only work queue; remove completed items in their commit.
  Docs describe current state; history belongs in Git.
- Curated names belong in `disasm/symbols.csv` when analysis begins. Use original
  file offsets or (segment, offset), never Amiga load addresses. Label evidence
  MEASURED, DERIVED or INFERRED.
- Owner decisions live in `docs/design.md` and change only with owner authorization.
- Commit directly to `main`, one verified coherent change per commit. Author and
  committer: Vesa Halttunen <vesuri@jormas.com>. Unsigned, `core.hooksPath=/dev/null`,
  no Co-Authored-By or other trailers. Keep Git configuration repository-local.
- Agent instructions belong here; `CLAUDE.md` only imports this file.

## Read first

| Document | Purpose |
|---|---|
| `README.md` | scope and setup checks |
| `docs/design.md` | intended approach, faithfulness boundary and owner decisions |
| `docs/source-inventory.md` | supplied archive identity, application facts and extraction layout |
| `docs/open-work.md` | deferred work and acceptance gates |

Use AmigaXDev's `docs/environment.md`, `docs/playbooks/new-port-playbook.md`,
`docs/reference/amiga-hardware.md`, `docs/reference/m68k-optimisation.md`,
`docs/reference/measurement-method.md`, `docs/reference/validation-harness.md`,
`docs/reference/headless-fsuae.md`, `docs/reference/port-toolchain.md` and
`docs/native-port-validation.md` for the shared workflow. Reuse the Vette! and
Alone in the Dark harnesses when implementation is authorized; do not copy their
measured game-specific assumptions into this project.
