# Port design

## Owner decisions

| Decision | Status |
|---|---|
| Port Macintosh Prince of Persia 2 to Amiga following Vette! and Alone in the Dark | Requested |
| Current scope | Initial setup only; no game analysis yet |
| Approach | Intended native original-instruction execution with Macintosh service implementation, following the reference ports; compatibility unverified |
| Exact Amiga CPU, chipset, memory budget and release requirements | Not yet decided |

## Faithfulness boundary

The original Macintosh execution will be the correctness reference. Preserve game
logic, data interpretation and decisions. Port-owned services handle Amiga display,
audio, input and platform integration. Any behavior-changing departure requires an
explicit owner decision and validation against the original. No runtime has been
copied or implemented, and no executable architecture is assumed proven.

Vette! and Alone in the Dark are the reuse references. Their inspected commits and
the shared playbook baseline are recorded in `manifest/references.tsv`.
