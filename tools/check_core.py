#!/usr/bin/env python3
"""Fresh original/native pairing and actual native rejection controls."""
import json,os,shutil,subprocess,tempfile
from pathlib import Path
from extract_original_data import ROOT
from check_core_result import check,restored
from check_core_preflight import check as preflight
from core_inputs import generate
from run_reference import run as reference_run

def main():
    os.chdir(ROOT)
    logs=Path(tempfile.mkdtemp(prefix='core-gate-',dir=ROOT/'tmp'))
    generate()
    os.environ['POP2_MAP']='1'
    reference=reference_run()
    preflight(reference)
    env=dict(os.environ,CORE='1',CORE_REFERENCE=str(reference))
    def build(flags='-DPOP2_CORE'):
        with (logs/('build-'+flags.replace(' ','_')+'.log')).open('w') as output:
            subprocess.run(['make','-C','amiga','VERIFY=1','EXTRA_DEFINES='+flags],check=True,stdout=output,stderr=subprocess.STDOUT)
    def launch(name,**changes):
        with (logs/(name+'.log')).open('w') as output:
            status=subprocess.run(['/bin/bash','amiga/run.sh'],env=dict(env,**changes),stdout=output,stderr=subprocess.STDOUT).returncode
        directory=ROOT/'amiga'/(ROOT/'amiga/.run-last.txt').read_text().strip()
        return status,directory,(directory/'gdb-out.log').read_text()
    build()
    status,positive,_=launch('positive');assert status==0,(status,logs)
    check(positive,reference)
    for name in ['bad-application','bad-plan']:
        status,directory,log=launch(name,CORE_CONTROL=name)
        assert status!=0 and 'CORE REASON CORE INPUT / SHA256' in log,(name,logs)
        assert (directory/'dh1/Shell.done').read_text().strip()=='SHELL FAIL'
    try:
        for flag,name,reason in [('POP2_BAD_RELOCATION','bad-byte','LOW MEMORY / ORIGINAL BYTE MISMATCH'),
                                ('POP2_UNKNOWN_TRAP','unknown-trap','UNIMPLEMENTED / TRAP'),
                                ('POP2_ILLEGAL','illegal-instruction','EXCEPTION / ILLEGAL INSTRUCTION'),
                                ('POP2_BAD_CODE_WRITE','code-write','CODE / UNEXPECTED WRITE')]:
            build('-DPOP2_CORE -D'+flag)
            status,directory,log=launch(name)
            assert status!=0 and 'CORE REASON '+reason in log,(name,logs)
            assert (directory/'dh1/Shell.done').read_text().strip()=='SHELL FAIL'
            if flag!='POP2_BAD_RELOCATION':restored(directory,log)
            if flag=='POP2_ILLEGAL':assert 'CORE EXCEPTION vector=4 ' in log
            if flag=='POP2_UNKNOWN_TRAP':assert 'CORE STOP trap=a123 ' in log
    finally:build()
    status,repeat,_=launch('repeat');assert status==0,(status,logs)
    check(repeat,reference)
    # Missing evidence, reordered traps and altered snapshots must fail the
    # very same checker that accepted a complete native run.
    fixture=logs/'evidence';shutil.copytree(repeat,fixture)
    good=(fixture/'gdb-out.log').read_text()
    mutations=[('',None), (good.replace('CODE 01+02EA','CODE 01+02EB'),None),
               (good.replace('heap=1','heap=0'),None),(good,'a5-core.bin'),(good,'CODE_1.bin')]
    for text,corrupt in mutations:
        (fixture/'gdb-out.log').write_text(text)
        if corrupt:
            original=(fixture/corrupt).read_bytes();changed=bytearray(original);changed[0]^=1;(fixture/corrupt).write_bytes(changed)
        try:check(fixture,reference)
        except (RuntimeError,ValueError):pass
        else:raise AssertionError('accepted incomplete or corrupted evidence')
        finally:
            if corrupt:(fixture/corrupt).write_bytes(original)
    observer=logs/'unreached.gdb';observer.write_text('break *pop2CoreStopped\ncontinue\necho CORE TIMEOUT CONTROL REACHED\\n\ncontinue\n')
    status,_,timeout_log=launch('timeout',GDBSCRIPT=str(observer),DEADLINE='20')
    assert status==124 and 'PROBE TIMEOUT' in (logs/'timeout.log').read_text() and 'CORE TIMEOUT CONTROL REACHED' in timeout_log
    # Conditional core integration must leave the earlier empty boot/quit valid.
    build('')
    status,_,_=launch('empty-scaffold',CORE='0');assert status==0
    build()
    status,final,_=launch('final');assert status==0;check(final,reference)
    tracked=subprocess.check_output(['git','ls-files'],text=True).splitlines()
    assert not any(p.startswith(('tmp/','ref/','build/','amiga/obj/','amiga/out/','amiga/.run')) for p in tracked)
    ignored=subprocess.check_output(['git','status','--ignored','--short'],text=True)
    assert '!! tmp/' in ignored and '!! ref/' in ignored and '!! build/' in ignored
    report={'result':'CORE GATE PASS','reference':str(reference.relative_to(ROOT)),
            'native':[str(p.relative_to(ROOT)) for p in [positive,repeat,final]],'native_rejection_controls':7,'evidence_rejection_controls':5}
    (logs/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print('CORE GATE PASS',logs.relative_to(ROOT),flush=True)
if __name__=='__main__':main()
