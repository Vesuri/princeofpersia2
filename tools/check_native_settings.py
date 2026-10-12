#!/usr/bin/env python3
"""Exercise changed native options across boot and D9's immutable-asset write stop."""
import argparse
import hashlib
import json
import os
import re
import shutil
import subprocess
import tempfile
from pathlib import Path
from extract_original_data import ROOT, DESTINATION, pins
from check_native_file_probe import verify as probe
from check_native_file_replay import verify as replay
from check_core_result import restored
from report_reference import require


def check(saved,reference,core_reference,written=None):
    replay(saved,core_reference)
    scratch=Path(tempfile.mkdtemp(prefix='settings-controls-',dir=ROOT/'tmp'))
    os.environ['CORE_REFERENCE']=str(core_reference)
    env=dict(os.environ,CORE='1',CORE_REFERENCE=str(core_reference),DEADLINE='180')
    for key in ('CORE_RESTART_FROM','CORE_CONTROL','CORE_FILE_REPLAY','GDBSCRIPT'):
        env.pop(key,None)

    def build(extra=''):
        with (scratch/('build-'+(extra or 'read')+'.log')).open('w') as output:
            subprocess.run(['make','-C','amiga','EXTRA_DEFINES=-DPOP2_CORE -DPOP2_FILE_PROBE '+extra],
                           cwd=ROOT,stdout=output,stderr=subprocess.STDOUT,check=True)

    def launch(label,**changes):
        with (scratch/(label+'.log')).open('w') as output:
            result=subprocess.run(['/bin/bash','amiga/run.sh'],cwd=ROOT,env=dict(env,**changes),
                                  stdout=output,stderr=subprocess.STDOUT)
        run=ROOT/'amiga'/(ROOT/'amiga/.run-last.txt').read_text().strip()
        return result.returncode,run

    try:
        if written is None:
            build('-DPOP2_SETTINGS_WRITE')
            status,written=launch('write');require(status==0,'SETTINGS WRITE RUN')
        probe(written,reference,'write')
        # Preserve the save fixture; compose a separate, explicitly hashed import
        # from native-written saves and native-written changed settings.
        imported=scratch/'import';(imported/'dh1/Data').mkdir(parents=True)
        manifest=json.loads((saved/'native-persistence.json').read_text())
        for relative in manifest:
            source=written if relative.startswith('Data/Prince2.opt') else saved
            shutil.copyfile(source/'dh1'/relative,imported/'dh1'/relative)
            manifest[relative]=hashlib.sha256((imported/'dh1'/relative).read_bytes()).hexdigest()
        (imported/'native-persistence.json').write_text(json.dumps(manifest,indent=2)+'\n')
        build()
        status,restarted=launch('restart',CORE_RESTART_FROM=str(imported))
        require(status==0,'SETTINGS RESTART RUN')
        require(json.loads((restarted/'dh1/restart-state.json').read_text())==manifest,'SETTINGS IMPORT MANIFEST')
        probe(restarted,reference,'restart')
        for relative,digest in manifest.items():
            require(hashlib.sha256((restarted/'dh1'/relative).read_bytes()).hexdigest()==digest,'SETTINGS READ-ONLY RESTART '+relative)
        build('-DPOP2_ASSET_WRITE_CONTROL')
        status,rejected=launch('immutable-write')
        log=(rejected/'gdb-out.log').read_text()
        require(status!=0 and 'CORE REASON FILES / UNSUPPORTED OPERATION' in log and
                'CORE STOP trap=a003 ' in log,'D9 WRITE LOUD STOP')
        closed=re.findall(r'^FILE PROBE open=0 read=0 close=(-?\d+) windows=',log,re.M)
        require('FILE WRITE mode=2 result=-1' in log and len(closed)==1 and int(closed[0])&0xffffffff==0xffffffff,'D9 WRITE REACHED')
        restored(rejected,log)
        original=(DESTINATION/'Data/MIDISnd.dat').read_bytes()[:16]
        expected=bytes([original[0]^1])+original[1:]
        require((rejected/'file-probe.bin').read_bytes()==expected,'D9 ATTEMPTED WRITE BUFFER')
        source_hashes=pins(ROOT/'manifest/runtime-data.sha256')
        for suffix in ('','.rsrc','.finfo'):
            relative='Data/MIDISnd.dat'+suffix
            require(hashlib.sha256((rejected/'dh1'/relative).read_bytes()).hexdigest()==source_hashes[relative],'D9 ORIGINAL PRESERVED')
        result={'result':'SETTINGS CONTROLS PASS','written':str(written.relative_to(ROOT)),
                'restarted':str(restarted.relative_to(ROOT)),'immutable_write':str(rejected.relative_to(ROOT))}
        (scratch/'report.json').write_text(json.dumps(result,indent=2)+'\n')
        print('SETTINGS CONTROLS PASS: changed native options survive boot; installed asset write stops; '+str(scratch.relative_to(ROOT)))
        return result
    finally:
        with (scratch/'build-normal.log').open('w') as output:
            subprocess.run(['make','-C','amiga','EXTRA_DEFINES=-DPOP2_CORE'],cwd=ROOT,stdout=output,stderr=subprocess.STDOUT,check=True)


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('saved','reference','core_reference'):parser.add_argument(name,type=Path)
    parser.add_argument('--written',type=Path)
    args=parser.parse_args()
    check(args.saved.resolve(),args.reference.resolve(),args.core_reference.resolve(),
          args.written.resolve() if args.written else None)
