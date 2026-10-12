#!/usr/bin/env python3
"""Two fresh reference runs, a live byte-guard control, and fail-closed report controls."""
from pathlib import Path
import os
import subprocess

from extract_original_data import ROOT, scratch_directory
from report_reference import analyze, report
from run_reference import run


def rejected(action,label):
    try:
        action()
    except ValueError:
        print('REJECTION PASS:',label)
    else:
        raise AssertionError('REFERENCE CONTROL ACCEPTED: '+label)


def main():
    first=run();second=run()
    for name in ('opening.ppm','traps.txt'):
        assert (first/name).read_bytes()==(second/name).read_bytes(), 'REPEATABILITY / '+name
    log=(first/'session.log').read_text()
    for marker in ('MILESTONE opening-text-1','REFERENCE_CAPTURED opening-text-1',
                   'SEED_RESULT value=00001234','PACK_CHECK flags=0000/0001','COPY_ROUTE nis=0007'):
        rejected(lambda:analyze(log.replace(marker,'REMOVED')),marker)
    rejected(lambda:analyze(log+'\nFAIL REFERENCE / timeout\n'),'timeout cannot pass')
    scratch=scratch_directory()
    (scratch/'session.log').write_text(log)
    frame=bytearray((first/'opening.ppm').read_bytes());frame[-1]^=1
    (scratch/'opening.ppm').write_bytes(frame)
    rejected(lambda:report(scratch),'changed framebuffer')
    # Host binary rejection precedes any volume creation or execution.
    previous=os.environ.get('MAME')
    os.environ['MAME']='/usr/bin/true'
    try:
        rejected(lambda:run(),'changed emulator')
    finally:
        if previous is None:os.environ.pop('MAME')
        else:os.environ['MAME']=previous
    run(control='bad-guard')
    tracked=subprocess.check_output(['git','ls-files','--','ref','tmp','.replaced'],cwd=ROOT,text=True)
    assert not tracked,'ORIGINAL/REFERENCE OUTPUT TRACKED'
    subprocess.run(['git','check-ignore','--','ref/','tmp/','.replaced/'],cwd=ROOT,check=True)
    subprocess.run(['git','status','--ignored','--short'],cwd=ROOT,check=True)
    print('REFERENCE GATE PASS: two identical captures and ordered trap logs; rejection controls passed')

if __name__=='__main__':main()
