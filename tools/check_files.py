#!/usr/bin/env python3
"""Reproduce paired original/native file, resource and persistence acceptance."""
import argparse
import json
import os
import subprocess
import tempfile
from pathlib import Path
from extract_original_data import ROOT
from core_inputs import generate as core_inputs
from file_replay import generate as replay_inputs
from run_reference import run as reference_run
from check_file_reference import check as reference_check
from check_file_transfers import check as transfer_check
from check_file_persistence import check as original_persistence
from check_persistence_controls import controls as original_controls
from check_native_file_replay import verify as replay_check
from check_native_file_probe import verify as file_probe_check
from check_native_resource_probe import verify as resource_check
from check_resource_controls import check as resource_controls
from check_native_persistence_controls import controls as native_controls
from check_resource_census import check as resource_census
from check_native_settings import check as settings_check


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--references',nargs=4,type=Path,metavar=('OPENING','REPEAT','SAVE','RESTART'))
    parser.add_argument('--core-reference',type=Path)
    args=parser.parse_args()
    os.chdir(ROOT)
    logs=Path(tempfile.mkdtemp(prefix='files-gate-',dir=ROOT/'tmp'))
    # Explicitly control profiles; inherited diagnostic variables must not turn
    # a positive run into a different workload.
    for key in ('POP2_RESTART_FROM','POP2_REFERENCE_CONTROL','POP2_FILE_ROUTINES',
                'CORE_RESTART_FROM','CORE_CONTROL','CORE_FILE_REPLAY','GDBSCRIPT'):
        os.environ.pop(key,None)
    if args.references:
        opening,repeat,saved,restarted=[p.resolve() for p in args.references]
    else:
        os.environ.update(POP2_FILES='1',POP2_RESOURCES='1',POP2_MAP='1')
        opening=reference_run();repeat=reference_run()
        os.environ['POP2_FILE_ROUTINES']='1'
        saved=reference_run()
        os.environ['POP2_RESTART_FROM']=str(saved)
        restarted=reference_run()
        os.environ.pop('POP2_RESTART_FROM');os.environ.pop('POP2_FILE_ROUTINES')
    reference_check([opening,repeat]);transfer_check(opening)
    original_persistence(saved,restarted);original_controls(saved,restarted)
    resource_census();core_inputs()
    core_reference=args.core_reference.resolve() if args.core_reference else opening
    env=dict(os.environ,CORE='1',CORE_REFERENCE=str(core_reference),DEADLINE='180')
    os.environ['CORE_REFERENCE']=str(core_reference)
    native={}

    def build(flags):
        with (logs/('build-'+flags.replace(' ','_')+'.log')).open('w') as stream:
            subprocess.run(['make','-C','amiga','VERIFY=1','EXTRA_DEFINES='+flags],stdout=stream,stderr=subprocess.STDOUT,check=True)

    def launch(label,**changes):
        with (logs/(label+'.log')).open('w') as stream:
            subprocess.run(['/bin/bash','amiga/run.sh'],env=dict(env,**changes),stdout=stream,stderr=subprocess.STDOUT,check=True)
        path=ROOT/'amiga'/(ROOT/'amiga/.run-last.txt').read_text().strip()
        native[label]=path
        return path

    try:
        for phase,reference in [('opening',opening),('save',saved),('restart',restarted)]:
            replay_inputs(reference,phase)
            build('-DPOP2_CORE -DPOP2_FILE_REPLAY')
            changes={'CORE_FILE_REPLAY':'1'}
            if phase=='restart':changes['CORE_RESTART_FROM']=str(native['save'])
            replay_check(launch(phase,**changes),core_reference)
        native_controls(native['save'],native['restart'],core_reference)
        build('-DPOP2_CORE -DPOP2_RESOURCE_PROBE')
        resource_check(launch('resources'),opening,core_reference)
        resource_controls(native['resources'],opening,core_reference)
        build('-DPOP2_CORE -DPOP2_FILE_PROBE')
        file_probe_check(launch('file-probe'),opening)
        settings=settings_check(native['save'],opening,core_reference)
    finally:
        build('-DPOP2_CORE')
    tracked=subprocess.check_output(['git','ls-files'],text=True).splitlines()
    assert not any(p.startswith(('tmp/','ref/','build/','amiga/obj/','amiga/out/','amiga/.run')) for p in tracked)
    ignored=subprocess.check_output(['git','status','--ignored','--short'],text=True)
    assert all('!! '+p+'/' in ignored for p in ('tmp','ref','build'))
    result={'result':'FILES GATE PASS',
            'reference':[str(p.relative_to(ROOT)) for p in (opening,repeat,saved,restarted)],
            'native':{k:str(v.relative_to(ROOT)) for k,v in native.items()},
            'resource_census':True,'settings':settings,'original_file_controls':5,'original_persistence_controls':3,
            'native_persistence_controls':7,'resource_controls':8}
    (logs/'report.json').write_text(json.dumps(result,indent=2)+'\n')
    print('FILES GATE PASS:',logs.relative_to(ROOT),flush=True)


if __name__=='__main__':main()
