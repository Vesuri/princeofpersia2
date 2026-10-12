#!/usr/bin/env python3
"""Run pinned shared Ghidra scripts on verified resource-relative CODE images."""
import hashlib
import os
from pathlib import Path
import subprocess
import shutil
import time
from static_map import ROOT, OUT, build
from extract_original_data import pins

def run():
    build()
    shared=Path(os.environ['AMIGA_GHIDRA_SCRIPTS'])
    for name,digest in pins(ROOT/'manifest/ghidra-scripts.sha256').items():
        if hashlib.sha256((shared/name).read_bytes()).hexdigest()!=digest:
            raise ValueError('GHIDRA SHARED SCRIPT HASH / '+name)
    home=Path(os.environ.get('GHIDRA_HOME',str(Path.home()/'.local/share/ghidra/ghidra_12.1_PUBLIC')))
    for name,digest in pins(ROOT/'manifest/ghidra-tool.sha256').items():
        if hashlib.sha256((home/name).read_bytes()).hexdigest()!=digest:raise ValueError('GHIDRA TOOL HASH / '+name)
    output=OUT/('ghidra-'+time.strftime('%Y%m%d-%H%M%S'))
    output.mkdir()
    inputs={name:hashlib.sha256((OUT/name).read_bytes()).hexdigest() for name in ('entrypoints.csv','ghidra-symbols.csv','map.json')}
    env=os.environ.copy()
    if not env.get('JAVA_HOME') and shutil.which('brew'):
        prefix=subprocess.check_output(['brew','--prefix','openjdk@21'],text=True).strip()
        env['JAVA_HOME']=str(Path(prefix)/'libexec/openjdk.jdk/Contents/Home')
    env['AMIGA_GHIDRA_ENTRYPOINTS']=str(OUT/'entrypoints.csv')
    for seg in range(1,24):
        if seg in (12,13,14):continue
        args=[str(home/'support/analyzeHeadless'),str(output),'Prince',
              '-import',str(OUT/f'CODE_{seg}.bin'),'-loader','BinaryLoader',
              '-processor','68000:BE:32:default','-cspec','default',
              '-scriptPath',str(shared),'-preScript','MacMarkEntries.java',
              '-postScript','MacDumpTraps.java',str(output/f'CODE_{seg}_traps.csv'),str(ROOT/'tmp/trap_names.lua'),
              '-postScript','MacApplyNames.java',str(OUT/'ghidra-symbols.csv'),
              '-postScript','ExportAuditListing.java',str(output/f'CODE_{seg}.txt'),
              '-postScript','DumpZeroPage.java',str(output/f'CODE_{seg}_lowmem.txt')]
        with (output/f'CODE_{seg}.log').open('w') as log:
            subprocess.run(args,env=env,stdout=log,stderr=subprocess.STDOUT,check=True)
        text=(output/f'CODE_{seg}.log').read_text()
        if 'ERROR ' in text or 'Exception' in text:raise ValueError('GHIDRA SCRIPT ERROR')
        if not (output/f'CODE_{seg}.txt').is_file():raise ValueError('GHIDRA EXPORT MISSING')
        print('GHIDRA segment',seg,'exported',flush=True)
    if any(hashlib.sha256((OUT/name).read_bytes()).hexdigest()!=digest for name,digest in inputs.items()):raise ValueError('GHIDRA INPUT CHANGED DURING ANALYSIS')
    (output/'receipt.json').write_text(__import__('json').dumps({'segments':20,'inputs':inputs})+'\n')
    (OUT/'ghidra-latest.txt').write_text(str(output.relative_to(ROOT))+'\n')
    print('GHIDRA COMPLETE',output.relative_to(ROOT))
if __name__=='__main__':run()
