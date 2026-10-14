"""Require a fresh Amiga session to load an unchanged profiling save and play."""
import argparse
import hashlib
import json
from pathlib import Path
from check_core_result import restored


def check(run,source,level):
    assert 1<=level<=14
    receipt=json.loads((source/'manifest.json').read_text())
    name=f'Profile {level:02}'
    for suffix in ('','.rsrc','.finfo'):
        filename=name+suffix
        assert hashlib.sha256((run/'dh1'/filename).read_bytes()).hexdigest()==receipt['files'][filename], 'PROFILE LOAD / UNCHANGED SAVE'
    build=json.loads((run/'build-receipt.json').read_text())
    assert {'-DPOP2_VERIFY','-DPOP2_GAMEPLAY_IMPORT_PROBE'}.issubset(build['flags'].split())
    for key,path in (('exe','dh1/PrinceOfPersia2'),('elf','PrinceOfPersia2.elf')):
        assert hashlib.sha256((run/path).read_bytes()).hexdigest()==build[key]
    log=(run/'gdb-out.log').read_text();restored(run,log,'a1200-030')
    for marker in ('CORE REASON GAMEPLAY / MAC SAVE IMPORT COMPLETE','GAMEPLAY FILE save=0 load=1','INTEGRATION AUDIT code=1 input=1'):
        assert log.count(marker+'\n')==1, 'PROFILE LOAD / COMPLETION '+marker
    for prefix in ('loaded-first','loaded'):
        a5=(run/(prefix+'-a5.bin')).read_bytes()
        assert int.from_bytes(a5[-0x4fd2:-0x4fd0],'big')==level, 'PROFILE LOAD / LEVEL'
    assert (run/'loaded-screen.bin').stat().st_size==307200
    print(f'PROFILE LOAD PASS: unchanged original {name}, first frame and gameplay frame, restored hardware')


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('run',type=Path);p.add_argument('source',type=Path);p.add_argument('level',type=int)
    a=p.parse_args();check(a.run,a.source,a.level)
