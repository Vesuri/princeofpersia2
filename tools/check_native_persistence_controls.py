#!/usr/bin/env python3
"""Reject damaged native persistence evidence after verifying intact copies."""
import argparse
import contextlib
import io
import json
import shutil
import struct
import tempfile
from pathlib import Path
from extract_original_data import ROOT
from check_native_persistence import check


def controls(written, restarted, core_reference):
    scratch = Path(tempfile.mkdtemp(prefix='native-persistence-controls-', dir=ROOT/'tmp'))
    source, target = scratch/'written', scratch/'restarted'
    shutil.copytree(written, source)
    shutil.copytree(restarted, target)
    check(source, target, core_reference)
    passed = []

    def reject(label, path, mutate, expected):
        before = path.read_bytes()
        damaged = mutate(before)
        assert damaged != before, 'ineffective control: '+label
        try:
            path.write_bytes(damaged)
            try:
                with contextlib.redirect_stdout(io.StringIO()):
                    check(source, target, core_reference)
            except ValueError as error:
                if expected not in str(error):
                    raise AssertionError('unrelated rejection: '+label+' / '+str(error)) from error
                passed.append(label)
            else:
                raise AssertionError('NATIVE PERSISTENCE CONTROL ACCEPTED: '+label)
        finally:
            path.write_bytes(before)

    flip = lambda b: bytes([b[0]^1])+b[1:]
    reject('missing restart completion', target/'gdb-out.log',
           lambda b: b.replace(b'FILE REPLAY completed=', b'FILE REPLAY incomplete='),
           'REPLAY POSITIVE COMPLETION')
    reject('changed flushed save', source/'dh1/Data/Fixture Save', flip, 'REPLAY FLUSHED WRITE')
    reject('changed restarted save', target/'dh1/Data/Fixture Save', flip, 'NATIVE PERSISTENCE RESTARTED HASH')
    reject('changed restarted resource fork', target/'dh1/Data/Prince.hof.rsrc',
           lambda b: b+b'X', 'NATIVE PERSISTENCE RESTARTED HASH')
    reject('changed import manifest', target/'dh1/restart-state.json',
           lambda b: json.dumps({**json.loads(b), 'Data/Fixture Save': '0'*64}).encode(),
           'NATIVE PERSISTENCE IMPORT MANIFEST')
    reject('damaged options metadata checksum', target/'dh1/Data/Prince2.opt.finfo',
           lambda b: b[:-1]+bytes([b[-1]^1]), 'NATIVE PERSISTENCE OPTIONS METADATA CHECKSUM')

    def alter_read(data):
        changed = bytearray(data)
        at = 8
        while at < len(data):
            trap, _, length = struct.unpack_from('>HHI', data, at)
            if trap == 0xa002 and length > 400:
                changed[at+400] ^= 1
                return bytes(changed)
            at += length
        raise AssertionError('no read payload for control')

    reject('changed restarted read payload', target/'file-replay-out.bin', alter_read,
           'REPLAY PAIRED PAYLOAD')
    # Re-run after restoration to prove the controls did not leave a broken fixture.
    check(source, target, core_reference)
    (scratch/'report.json').write_text(json.dumps({'controls': passed}, indent=2)+'\n')
    print(f'NATIVE PERSISTENCE CONTROLS PASS: {len(passed)}; {scratch.relative_to(ROOT)}')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('written', 'restarted', 'core_reference'):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    controls(args.written.resolve(), args.restarted.resolve(), args.core_reference.resolve())
