"""Check the original resetLoop store and its native low-memory relocation.

The native debugger must stop after the real CODE 20 instruction, not on a
replacement helper entry. Dumps and logs remain ignored, as with other gates.
"""
import argparse
import gzip
import hashlib
import json
import re
from pathlib import Path
from extract_original_data import ROOT, pins


def check(original, native):
    pending = None
    completed = False
    counts = {'null': 0, 'allocated': 0}
    digest = hashlib.sha256()
    log_path = original/'session.log'
    stream = log_path.open('rb') if log_path.exists() else gzip.open(original/'session.log.gz', 'rb')
    with stream:
        for raw in stream:
            digest.update(raw)
            line = raw.decode('utf-8', errors='replace')
            if line.strip() == 'AUDIO_DEMO_COMPLETE':
                completed = True
            before = re.search(r'WAVE_LOOP_STORE pointer=([0-9A-F]+) value=([0-9A-F]+)', line)
            after = re.search(r'WAVE_LOOP_STORED pointer=([0-9A-F]+) value=([0-9A-F]+)', line)
            if before:
                assert pending is None, 'STORE / MISSING ORIGINAL RETURN'
                pending = tuple(int(v, 16) for v in before.groups())
            elif after and pending is not None:
                observed = tuple(int(v, 16) for v in after.groups())
                assert observed == pending, 'STORE / ORIGINAL VALUE'
                counts['null' if pending[0] == 0 else 'allocated'] += 1
                pending = None
    assert digest.hexdigest() == pins(ROOT/'manifest/indirect-store-reference.sha256')['session.log'], 'STORE / ORIGINAL TRACE PIN'
    assert completed and pending is None and all(counts.values()), 'STORE / ORIGINAL COVERAGE'
    log = (native/'gdb.log').read_text()
    def register(name):
        values = re.findall(r'^'+name+r'\s+0x([0-9a-f]+)\b', log, re.M)
        assert len(values) == 1, 'STORE / REGISTER '+name
        return int(values[0], 16)
    assert register('a0') == 0 and register('d0') == 0xffffffff, 'STORE / NATIVE INPUT'
    before_sr = re.findall(r'STORE_BEFORE sr=([0-9a-f]+)', log)
    assert len(before_sr) == 1 and (int(before_sr[0], 16) & ~15) == (register('sr') & ~15), 'STORE / PRESERVED STATUS'
    assert register('sr') & 15 == 8, 'STORE / MOVE CONDITION CODES'
    fast = (native/'after-fast.bin').read_bytes()
    chip = (native/'after-chip.bin').read_bytes()
    assert len(fast) == 0x800000 and len(chip) == 0x200000, 'STORE / CAPTURE EXTENT'
    pc = register('pc')
    assert fast[pc-0x200000:pc-0x200000+4] == bytes.fromhex('4cdf0c00'), 'STORE / ORIGINAL CONTINUATION'
    low = register('a5')+0x2020-0x200000
    assert fast[low:low+4] == bytes.fromhex('ffffffff'), 'STORE / VIRTUAL LOW MEMORY'
    assert fast[pc-0x200000-4:pc-0x200000] == bytes.fromhex('aff94e71'), 'STORE / RELOCATED INSTRUCTION'
    assert chip[0x20:0x24] == (native/'store-vector.bin').read_bytes(), 'STORE / VECTOR RESTORATION'
    vector = int.from_bytes(chip[0x20:0x24], 'big')
    assert 0x200000 <= vector < 0xa00000, 'STORE / AMIGA PRIVILEGE VECTOR'
    receipt = json.loads((native/'receipt.json').read_text())
    assert set(receipt['flags'].split()) == {'-DPOP2_'+v for v in ('CORE','DISPLAY','INPUT','UI','AUDIO')}, 'STORE / RELEASE BUILD'
    assert hashlib.sha256((native/'game/PrinceOfPersia2').read_bytes()).hexdigest() == receipt['exe'], 'STORE / EXECUTABLE IDENTITY'
    result = {'result': 'INDIRECT STORE PASS', 'original_stores': counts, 'exe': receipt['exe'],
              'native_null_value': 0xffffffff, 'move_nzvc': 8, 'amiga_vector_preserved': True}
    (native/'indirect-store.json').write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps(result))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('original', type=Path)
    parser.add_argument('native', type=Path)
    args = parser.parse_args()
    check(args.original, args.native)
