"""Check original later-cutscene offscreen layouts against shared GWorld records."""
import argparse
import hashlib
import re
import struct
import subprocess
from pathlib import Path
from extract_original_data import ROOT,pins


def check(run,ending=False):
    for name,digest in pins(ROOT/('manifest/gameplay-ending-worlds.sha256' if ending else 'manifest/gameplay-worlds.sha256')).items():
        assert hashlib.sha256((run/name).read_bytes()).hexdigest()==digest,'WORLD / HASH '+name
    log=(run/'session.log').read_text()
    assert log.count('GAMEPLAY_ENDING_COMPLETE\n' if ending else 'GAMEPLAY_DISCOVERY_COMPLETE\n')==1 and 'FAIL REFERENCE' not in log,'WORLD / COMPLETION'
    calls=[int(n) for n in re.findall(r'^GAMEPLAY_WORLD_RETURN id=(\d+)$',log,re.M)]
    assert calls==list(range(1,len(calls)+1)) and calls,'WORLD / CALL ORDER'
    boxes=[struct.unpack('>4h',(run/f'world-{n}-bounds.bin').read_bytes()) for n in calls]
    if not ending: assert set(boxes)=={(0,0,384,512),(0,0,384,510),(0,0,87,510),(0,0,365,510),(0,0,365,365),(0,0,353,239),(0,0,225,510)},'WORLD / COVERAGE'
    exe=ROOT/'tmp/probe-gameplay-worlds'
    subprocess.run(['c++','-std=c++17','-O2','-I'+str(ROOT/'src'),str(ROOT/'tools/probe_gworld_records.cpp'),'-o',str(exe)],check=True)
    generated=subprocess.check_output([str(exe)],input=''.join(' '.join(map(str,b))+'\n' for b in boxes).encode())
    assert len(generated)==len(calls)*158,'WORLD / RECORD SIZE'
    for i,n in enumerate(calls):
        expected=(run/f'world-{n}-map.bin').read_bytes();actual=generated[i*158+108:(i+1)*158]
        assert len(expected)==50,'WORLD / CAPTURE SIZE'
        assert actual[4:42]==expected[4:42] and actual[46:]==expected[46:],'WORLD / PIXMAP FIELDS'
    print(f'ORIGINAL GWORLD PASS: {len(calls)} allocations, {len(set(boxes))} layouts, all non-pointer PixMap fields')


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('run',type=Path)
    p.add_argument('--ending',action='store_true')
    a=p.parse_args();check(a.run,a.ending)
