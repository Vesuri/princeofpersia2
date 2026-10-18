"""Verify the original volume-date residue in the first-room file parameter block."""
import argparse
import hashlib
import re
from pathlib import Path

from check_integration_result import normalized_reference_a5, require, reference_state
from extract_original_data import ROOT, pins


def check(reference, baseline):
    recorded = pins(ROOT/'manifest/integration-file-scratch.sha256')
    for run in (reference, baseline):
        require(str((run/'first-room-a5.bin').resolve().relative_to(ROOT)) in recorded,
                'FILE SCRATCH UNPINNED RUN')
    require(str((reference/'session.log').resolve().relative_to(ROOT)) in recorded,
            'FILE SCRATCH UNPINNED TRACE')
    for name, digest in recorded.items():
        require(hashlib.sha256((ROOT/name).read_bytes()).hexdigest() == digest,
                'FILE SCRATCH INPUT PIN '+name)
    log = (reference/'session.log').read_text()
    require('FAIL REFERENCE' not in log, 'FILE SCRATCH ORIGINAL ERROR')
    require(log.count('DISPLAY_ROUTE_CAPTURED first-room\n') == 1,
            'FILE SCRATCH FIRST ROOM COMPLETION')
    prefix = log.split('DISPLAY_ROUTE_CAPTURED first-room\n')[0]
    a5s = re.findall(r'^FIRST_ROOM_STATE a5=([0-9A-F]+) context=', prefix, re.M)
    require(len(a5s) == 1, 'FILE SCRATCH A5 RECORD')
    pb = int(a5s[0], 16) - 0xb9a
    rows = re.findall(r'^FILE_CALL id=(\d+) pc=([0-9A-F]+) trap=([0-9A-F]+) selector=([0-9A-F]+) pb=([0-9A-F]+)$', prefix, re.M)
    rows = [r for r in rows if int(r[4], 16) == pb]
    volumes = [i for i, r in enumerate(rows) if r[2] == 'A207']
    require(bool(volumes), 'FILE SCRATCH VOLUME CALL')
    last = volumes[-1]
    require(f'FILE_RETURN id={rows[last][0]} result=0000\n' in prefix,
            'FILE SCRATCH VOLUME COMPLETION')
    volume = (reference/f'file-{rows[last][0]}-after.bin').read_bytes()
    require(len(volume) == 128, 'FILE SCRATCH PB EXTENT')
    residue = volume[34:36]
    require(residue != bytes(2), 'FILE SCRATCH POSITIVE DATE')
    seeks = 0
    for ordinal, _, trap, _, _ in rows[last+1:]:
        require(f'FILE_RETURN id={ordinal} result=0000\n' in prefix,
                'FILE SCRATCH CALL COMPLETION')
        for phase in ('before', 'after'):
            data = (reference/f'file-{ordinal}-{phase}.bin').read_bytes()
            require(len(data) == 128 and data[34:36] == residue,
                    'FILE SCRATCH RETAINED DATE')
        if trap == 'A044':
            seeks += 1
            require(data[36:38] == bytes(2), 'FILE SCRATCH SETFPOS LOW WORD')
    require(seeks > 0, 'FILE SCRATCH SETFPOS CONTROL')
    raw = (reference/'first-room-a5.bin').read_bytes()
    old = (baseline/'first-room-a5.bin').read_bytes()
    require(raw[-0xb78:-0xb76] == residue, 'FILE SCRATCH SNAPSHOT PROVENANCE')
    require(old[-0xb78:-0xb76] != residue, 'FILE SCRATCH DIFFERENT DATE CONTROL')
    require(normalized_reference_a5(raw) == normalized_reference_a5(old),
            'FILE SCRATCH OTHER A5 BYTES')
    for name in ('first-room-context.bin', 'first-room.indices', 'first-room.rgb'):
        require((reference/name).read_bytes() == (baseline/name).read_bytes(),
                'FILE SCRATCH MATCH '+name)
    reference_state(reference)
    reference_state(baseline)
    # Neighbouring fields and gameplay state must still affect the pinned hash.
    expected = normalized_reference_a5(raw)
    for distance in (0xb79, 0xb76, 0xb75, 0x18fc, 0x5208, 0x4fd2):
        altered = bytearray(raw); altered[-distance] ^= 1
        require(normalized_reference_a5(altered) != expected,
                'FILE SCRATCH CORRUPTION CONTROL')
    print(f'FILE SCRATCH PASS: volume call {rows[last][0]}, {seeks} subsequent seeks; '
          'matching first-room state/pixels/palette; six corruption controls; '
          'normalized SHA-256 '+hashlib.sha256(expected).hexdigest())


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('reference', type=Path)
    parser.add_argument('baseline', type=Path)
    args = parser.parse_args()
    check(args.reference, args.baseline)
