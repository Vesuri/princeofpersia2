#!/usr/bin/env python3
"""Run one isolated headless Macintosh reference and require positive evidence."""
import argparse
import hashlib
import os
from pathlib import Path
import shutil
import subprocess
import time
import uuid

from extract_original_data import ROOT, pins
from prepare_reference import prepare
from reference_metadata import generate


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def run(name=None, control=None):
    mame=Path(shutil.which(os.environ.get('MAME','mame')) or '')
    if not mame.is_file():
        raise ValueError('REFERENCE / MAME NOT FOUND')
    expected=pins(ROOT/'manifest/reference-tools.sha256')['mame']
    if digest(mame)!=expected:
        raise ValueError('REFERENCE / MAME HASH MISMATCH')
    name=name or time.strftime('%Y%m%d-%H%M%S-')+uuid.uuid4().hex[:8]
    if not name or any(c not in 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_' for c in name):
        raise ValueError('REFERENCE / INVALID RUN NAME')
    destination=ROOT/'ref/mame/runs'/name
    prepare(destination,'755')
    generate(destination/'metadata.lua')
    subprocess.run(['python3','tools/check_mame_literals.py','--syntax-wrapper',str(destination/'syntax.lua')],cwd=ROOT,check=True)
    env=os.environ.copy()
    env.update(SDL_VIDEODRIVER='dummy',POP2_REFERENCE_RUN=str(destination.relative_to(ROOT)))
    if control:env['POP2_REFERENCE_CONTROL']=control
    args=[str(mame),'maciici','-noreadconfig','-ramsize','8M','-nbc','mdc48',
          '-hard',str(destination/'volume.hd'),'-rompath','ref/mame/roms',
          '-nvram_directory',str(destination/'nvram'),'-cfg_directory',str(destination/'cfg'),
          '-snapshot_directory',str(destination/'snap'),'-video','none','-sound','none','-window',
          '-nothrottle','-skip_gameinfo','-debug','-debugger','none','-oslog','-nosamples',
          '-autoboot_delay','0','-autoboot_script',str(destination/'syntax.lua'),'-seconds_to_run','180']
    pidfile=destination/'mame.pid'
    with (destination/'session.log').open('wb') as output:
        process=subprocess.Popen(args,cwd=ROOT,env=env,stdout=output,stderr=subprocess.STDOUT)
        pidfile.write_text(str(process.pid)+'\n')
        try:
            result=process.wait(timeout=240)
        except BaseException:
            # Use the shared identity-checked helper; never kill by process name.
            subprocess.run(['/bin/bash','-c','. amiga/env.sh; . "$FSUAE_COMMON"; fsuae_kill_recorded "$1" mame','reference',str(pidfile)],cwd=ROOT,check=True)
            process.wait()
            raise
    if result:
        raise ValueError('REFERENCE / MAME EXIT '+str(result))
    if control:
        log=(destination/'session.log').read_text()
        if ('FAIL REFERENCE' not in log or 'PROBE BYTES' not in log) or 'REFERENCE_CAPTURED' in log:
            raise ValueError('REFERENCE / BYTE-GUARD CONTROL DID NOT REJECT')
        print('REFERENCE BYTE-GUARD CONTROL PASS:',destination.relative_to(ROOT))
        return destination
    from report_reference import report
    report(destination)
    return destination

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--name')
    parser.add_argument('--control',choices=['bad-guard'])
    args=parser.parse_args()
    run(args.name,args.control)
