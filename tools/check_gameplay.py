"""Repeat an original-paired session route on the current gameplay target."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile
from check_gameplay_restart import check as check_restart, controls as restart_controls
from check_gameplay_save import check as check_save, controls as save_controls
from gameplay_saved_file import export as export_save
from check_integration import compress_completed_volume
from extract_original_data import ROOT, pins
from run_reference import run as reference_run
import hashlib


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--route',choices=('restart','save-load'),default='restart')
    parser.add_argument('--references',nargs=2,type=Path)
    parser.add_argument('--native-runs',nargs=2,type=Path,
                        help='check completed diagnostic runs instead of launching')
    args=parser.parse_args()
    check,controls=(check_restart,restart_controls) if args.route=='restart' else (check_save,save_controls)
    label='restart' if args.route=='restart' else 'save'
    os.chdir(ROOT)
    logs=Path(tempfile.mkdtemp(prefix='gameplay-'+label+'-gate-',dir=ROOT/'tmp'))
    for key in list(os.environ):
        if key.startswith(('POP2_','CORE_','UI_','INPUT_','DISPLAY_','AUDIO_',
                           'INTEGRATION_','GAMEPLAY_')) or key in ('CORE','GDBSCRIPT','SNAPSHOT_AFTER'):
            os.environ.pop(key)
    references=[p.resolve() for p in args.references] if args.references else []
    if not references:
        os.environ.update(POP2_GAMEPLAY=args.route,POP2_DISPLAY_ROUTE='1')
        for _ in range(2):
            reference=reference_run()
            if args.route=='save-load':export_save(reference)
            compress_completed_volume(reference)
            references.append(reference)
        os.environ.pop('POP2_GAMEPLAY');os.environ.pop('POP2_DISPLAY_ROUTE')
    assert len(set(references))==2,'GAMEPLAY / DISTINCT ORIGINAL RUNS REQUIRED'
    for reference in references:
        log=(reference/'session.log').read_text()
        endpoint='first_frames=2' if args.route=='restart' else 'loaded_frames=1'
        assert log.count('GAMEPLAY_REFERENCE_COMPLETE route='+args.route+' '+endpoint+'\n')==1
        if args.route=='restart':
            assert log.count('GAMEPLAY_MENU ordinal=1 command=01930009\n')==1
        else:
            export_save(reference)
            assert log.count('GAMEPLAY_VOLUME_FLUSHED\n')==1
            assert log.count('GAMEPLAY_MENU ordinal=1 command=01910005\n')==1
            assert log.count('GAMEPLAY_MENU ordinal=2 command=01910004\n')==1
        assert 'FAIL REFERENCE' not in log
        for name,digest in pins(ROOT/('manifest/gameplay-'+label+'-reference.sha256')).items():
            assert hashlib.sha256((reference/name).read_bytes()).hexdigest()==digest,name
    if args.route=='save-load':
        import re
        key_traces=[re.findall(r'^GAMEPLAY_KEYS .*$',(p/'session.log').read_text(),re.M) for p in references]
        assert len(key_traces[0])==26 and key_traces[0]==key_traces[1],'SAVE ORIGINAL INPUT REPEAT'
    runs=[p.resolve() for p in args.native_runs] if args.native_runs else []
    if not runs:
        flags='-DPOP2_CORE -DPOP2_DISPLAY -DPOP2_INPUT -DPOP2_UI -DPOP2_AUDIO -DPOP2_INTEGRATION_PROBE -DPOP2_INTEGRATION_ESCAPE -DPOP2_GAMEPLAY_RESTART_PROBE'
        if args.route=='save-load':flags+=' -DPOP2_GAMEPLAY_SAVE_PROBE'
        try:
            with (logs/'build.log').open('w') as out:
                subprocess.run(['make','-C','amiga','VERIFY=1','EXTRA_DEFINES='+flags],stdout=out,stderr=subprocess.STDOUT,check=True)
            for ordinal in range(2):
                env=dict(os.environ,CORE='1',GAMEPLAY_REFERENCE=str(references[0]),
                         AMIGA_SETUP='a1200-030',GDBSCRIPT=str(ROOT/('amiga/gameplay_'+label+'.gdb')),DEADLINE='600')
                env['GAMEPLAY_RESTART' if args.route=='restart' else 'GAMEPLAY_SAVE']='1'
                with (logs/('native-'+str(ordinal)+'.log')).open('w') as out:
                    subprocess.run(['/bin/bash','amiga/run.sh'],env=env,stdout=out,stderr=subprocess.STDOUT,check=True)
                runs.append(ROOT/'amiga'/(ROOT/'amiga/.run-last.txt').read_text().strip())
        finally:
            with (logs/'release-build.log').open('w') as out:
                subprocess.run(['make','release','VERIFY=0'],stdout=out,stderr=subprocess.STDOUT,check=True)
    assert len(set(runs))==2,'GAMEPLAY / DISTINCT NATIVE RUNS REQUIRED'
    for run in runs:
        check(run,references[0],'a1200-030')
    for frame_label in (('first-room','restart') if args.route=='restart' else ('first-room','loaded-first','loaded')):
        for suffix in ('screen','planes','colors'):
            name=frame_label+'-'+suffix+'.bin'
            assert (runs[0]/name).read_bytes()==(runs[1]/name).read_bytes(),name
        for suffix,extent in [('context',slice(0x2180,0x21ba)),('a5',slice(-0x18fc,-0x18f8))]:
            name=frame_label+'-'+suffix+'.bin'
            assert (runs[0]/name).read_bytes()[extent]==(runs[1]/name).read_bytes()[extent],name
    controls(runs[1],references[0],'a1200-030')
    tracked=subprocess.check_output(['git','ls-files'],text=True).splitlines()
    assert not any(p.startswith(('tmp/','ref/','build/','amiga/.run','amiga/obj/','amiga/out/')) for p in tracked)
    ignored=subprocess.check_output(['git','status','--ignored','--short'],text=True)
    assert all('!! '+p+'/' in ignored for p in ('tmp','ref','build'))
    report={'result':'GAMEPLAY '+args.route.upper()+' GATE PASS','references':[str(p.relative_to(ROOT)) for p in references],
            'native':[str(p.relative_to(ROOT)) for p in runs],'controls':15 if args.route=='restart' else 22}
    (logs/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print('GAMEPLAY '+args.route.upper()+' GATE PASS:',logs.relative_to(ROOT))


if __name__=='__main__':
    main()
