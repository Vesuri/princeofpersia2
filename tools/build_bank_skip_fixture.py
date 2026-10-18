"""Reproduce the pinned BankRLELR fixture from original code and captured calls."""
import argparse
import hashlib
import json
from pathlib import Path
from extract_original_data import ROOT, pins
import _amigaxdev
from amigaxdev.mac.resource_fork import read_resource_fork


def build(out):
    source = ROOT/'tmp/runtime-data/Prince of Persia 2.rsrc'
    assert hashlib.sha256(source.read_bytes()).hexdigest() == pins(ROOT/'manifest/runtime-data.sha256')[source.name]
    code = next(r.body for r in read_resource_fork(source) if r.kind == b'CODE' and r.rid == 17)
    original = code[0x90:0x140]+bytes.fromhex('4e75')  # Authored RTS after original MOVEM restore.
    assert hashlib.sha256(original).hexdigest() == '6c77739e83b8d42c1398a9e7418999e35e1a37ba606741b3aab1d80957ea6f11'
    replacement = bytearray(original)
    # Authored SUBA.W D5,A2; SUBQ.L #1,A2; MOVEQ #-1,D5; BRA.B original advance.
    replacement[0x10e-0x90:0x116-0x90] = bytes.fromhex('94c5538a7aff600c')
    census = pins(ROOT/'manifest/performance-68020-rle-census.sha256')
    def captured(name):
        data = (ROOT/name).read_bytes()
        assert hashlib.sha256(data).hexdigest() == census[name], name
        return data
    calls = json.loads(captured('tmp/item15-rle-census/calls.json'))
    encoded = bytearray(); cases = []
    for call in calls:
        if call['kind'] != 1:
            continue
        data = captured('amiga/.run-BW5Tb2/rle-'+str(call['n'])+'.bin')
        assert hashlib.sha256(data).hexdigest() == call['digest']
        cases.append([len(encoded), call['w'], call['h'], call['left'], call['mode']])
        encoded.extend(data)
    assert len(cases) == 14
    for n in range(1, 129):
        for left, width in sorted({(0,n), (0,1), (n-1,1), (n//2,n-n//2)}):
            cases.append([len(encoded), width, 3, left, 0x1f1f])
            encoded.extend(bytes((0,2,127+n,0))*3)
        row = bytes((1,7,255,127+n,0,0x81,0xf8))
        for left, width in ((0,n+4), (1,n+2), (2,n), (n+1,3)):
            cases.append([len(encoded), width, 2, left, 0x0f0f])
            encoded.extend((len(row).to_bytes(2,'big')+row)*2)
    assert len(cases) == 1034
    def array(name, data):
        return 'static const unsigned char '+name+'[]={'+','.join(map(str,data))+'};\n'
    header = array('original', original)+array('replacement', replacement)+array('encoded', encoded)
    header += 'static const unsigned long cases[][5]={'+','.join('{'+','.join(map(str,c))+'}' for c in cases)+'};\n#define CASE_COUNT 1034\n'
    files = {'original.bin':original, 'replacement.bin':replacement, 'encoded.bin':encoded,
             'cases.json':(json.dumps(cases)+'\n').encode(), 'fixture.h':header.encode(),
             'cases.lua':('return {'+','.join('{'+','.join(map(str,c))+'}' for c in cases)+'}\n').encode()}
    # Preserve existing differing output; select a fresh directory instead.
    out.mkdir(parents=True, exist_ok=True)
    for name, data in files.items():
        path = out/name
        assert not path.exists() or path.read_bytes() == data, ('BANK / EXISTING OUTPUT', path)
    for name, data in files.items():
        (out/name).write_bytes(data)
    print('BANK FIXTURE BUILT: 1034 inputs, 1468 CCR cases')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    build(parser.parse_args().output)
