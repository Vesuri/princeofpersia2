#!/usr/bin/env python3
"""Two original runs, repeatable executable images, and rejecting controls."""
import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
from check_static_map import check, require
from static_map import ROOT, OUT, build
from run_reference import run

def main():
    p=argparse.ArgumentParser();p.add_argument('--runs',nargs=2,type=Path);a=p.parse_args()
    mapping=build()
    ghidra=ROOT/(OUT/'ghidra-latest.txt').read_text().strip()
    receipt=json.loads((ghidra/'receipt.json').read_text())
    require(receipt['segments']==20,'Ghidra segment count')
    for name,digest in receipt['inputs'].items():
        require(hashlib.sha256((OUT/name).read_bytes()).hexdigest()==digest,'stale Ghidra export '+name)
    if a.runs:directories=[x.resolve() for x in a.runs]
    else:
        os.environ['POP2_MAP']='1';directories=[run(),run()]
    reports=[check(d,mapping) for d in directories]
    require(reports[0]==reports[1],'two-run analysis facts')
    names=['MDRV-encrypted.bin','MDRV-decrypted.bin','MDRV-expanded.bin','MDRV-installed.bin','MDRV-final.bin','a5-initial.bin','runtime-stubs.bin']
    names += sorted(f.name for f in directories[0].glob('map-CODE_*-after.bin'))
    for name in names:require((directories[0]/name).read_bytes()==(directories[1]/name).read_bytes(),'two-run image '+name)
    scratch=Path(tempfile.mkdtemp(prefix='map-controls-',dir=ROOT/'tmp'))
    for f in directories[0].iterdir():
        if f.name in ('session.log','opening.ppm') or f.suffix=='.bin':shutil.copyfile(f,scratch/f.name)
    check(scratch,mapping)
    controls=0
    def rejects(action):
        nonlocal controls
        try:action()
        except ValueError:controls+=1
        else:raise ValueError('STATIC/LIVE MAP / negative control accepted')
    log=(scratch/'session.log').read_text()
    for marker in ('LIVE_MAP_COMPLETE','STUBS_COMPLETE','DECRYPT_COMPLETE','WRITE_CONTROL'):
        (scratch/'session.log').write_text(log.replace(marker,'REMOVED',1))
        rejects(lambda:check(scratch,mapping))
    (scratch/'session.log').write_text(log)
    for name in ('a5-initial.bin','MDRV-decrypted.bin','MDRV-installed.bin','map-CODE_2-after.bin'):
        file=scratch/name;original=file.read_bytes();altered=bytearray(original);altered[len(altered)//2]^=1;file.write_bytes(altered)
        rejects(lambda:check(scratch,mapping));file.write_bytes(original)
    altered=copy.deepcopy(mapping);altered['traps']=[row for row in altered['traps'] if tuple(row[:2])!=(1,0x2ea)]
    rejects(lambda:check(scratch,altered))
    # Synthetic original-region write: the checker must not permit an opcode patch.
    import re
    base=int(re.search(r'LIVE_MAP seg=2 base=(\w+)',log)[1],16)
    delta=int(re.search(r'WRITE_CONTROL .* delta=(\w+)',log)[1],16)
    injection=f'CODEWRITE CODE_2 pc={base+0x10:08X} addr={base+delta+0x10:08X} size=10 data=00004E71\n'
    (scratch/'session.log').write_text(log.replace('LIVE_MAP_COMPLETE',injection+'LIVE_MAP_COMPLETE'))
    rejects(lambda:check(scratch,mapping))
    (scratch/'session.log').write_text(log)
    file=scratch/'map-CODE_2-after.bin';held=scratch/'held-CODE_2.bin'
    file.rename(held)
    rejects(lambda:check(scratch,mapping));held.rename(file)
    lines=log.splitlines(keepends=True)
    first=next(i for i,line in enumerate(lines) if line.startswith('CODEWRITE STUB '))
    (scratch/'session.log').write_text(''.join(lines[:first]+lines[first+1:]))
    rejects(lambda:check(scratch,mapping))
    tracked=subprocess.check_output(['git','ls-files','tmp','ref','.replaced'],cwd=ROOT,text=True)
    require(not tracked,'derived files tracked')
    for directory in directories+[OUT,scratch]:
        subprocess.run(['git','check-ignore','-q',str(directory)],cwd=ROOT,check=True)
    subprocess.run(['git','status','--ignored','--short'],cwd=ROOT,check=True)
    print('STATIC MAP GATE PASS: two runs, all image hashes identical,',controls,'rejection controls; originals/derived output ignored')

if __name__=='__main__':main()
