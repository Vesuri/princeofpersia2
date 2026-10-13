"""Positive empty boot/quit plus real failed-exit and deadline controls."""
import hashlib,json,os,subprocess,tempfile
from pathlib import Path
from scaffold_result import check
from package_release import package
ROOT=Path(__file__).resolve().parents[1]
def run(command,log,env=None):
    with log.open('w') as output:
        return subprocess.run(command,cwd=ROOT,env=env,stdout=output,stderr=subprocess.STDOUT).returncode

def main():
    logs=Path(tempfile.mkdtemp(prefix='scaffold-gate-',dir=ROOT/'tmp'))
    def launch(name,env=None):
        status=run(['/bin/bash','amiga/run.sh'],logs/(name+'.log'),env)
        result=ROOT/'amiga'/(ROOT/'amiga/.run-last.txt').read_text().strip()
        return status,result
    status,positive=launch('positive')
    assert status==0,(status,logs/'positive.log')
    check(positive)
    # Missing, malformed or partial evidence cannot pass the result checker.
    fixture=logs/'evidence';(fixture/'dh1').mkdir(parents=True)
    valid=(positive/'gdb-out.log').read_text()
    (fixture/'logs').mkdir()
    (fixture/'logs/debug.uae').write_bytes((positive/'logs/debug.uae').read_bytes())
    for label,log,marker in [('missing-probe','', 'SHELL PASS'),
                             ('wrong-stage',valid.replace('stage=3','stage=2'),'SHELL PASS'),
                             ('missing-shell',valid,''),('failed-shell',valid,'SHELL FAIL')]:
        (fixture/'gdb-out.log').write_text(log);(fixture/'dh1/Shell.done').write_text(marker)
        try:check(fixture)
        except RuntimeError:pass
        else:raise AssertionError('accepted '+label)
    # Exercise the actual CRT result and Shell branch on the same restore path.
    try:
        assert run(['make','-C','amiga','VERIFY=1','EXTRA_DEFINES=-DPOP2_FAIL_EXIT'],logs/'build-fail.log')==0
        status,failed=launch('failed-exit')
        assert status!=0 and 'SCAFFOLD PROBE PASS' in (failed/'gdb-out.log').read_text()
        assert (failed/'dh1/Shell.done').read_text().strip()=='SHELL FAIL'
    finally:
        assert run(['make','-C','amiga','VERIFY=1','EXTRA_DEFINES='],logs/'build-normal.log')==0
    # Both binaries restore; only one returns success. Revalidate normal build.
    status,restored=launch('normal-again')
    assert status==0
    # A debugger waiting beyond the completion breakpoint must time out, not pass.
    observer=logs/'unreached.gdb'
    observer.write_text('break *pop2ScaffoldRestored\ncontinue\ncontinue\n')
    status,timed=launch('timeout',dict(os.environ,GDBSCRIPT=str(observer),DEADLINE='3'))
    assert status==124 and 'SCAFFOLD / PROBE TIMEOUT' in (logs/'timeout.log').read_text()
    assert run(['make','-C','whdload'],logs/'slave.log')==0
    archive,files=package();first=archive.read_bytes()
    again,_=package();assert first==again.read_bytes(),'nondeterministic LHA'
    # Independent Lhasa decoder verifies headers, CRCs and each exact member.
    unpack=logs/'unpack';unpack.mkdir()
    subprocess.run(['lha','x',str(archive)],cwd=unpack,check=True,stdout=subprocess.DEVNULL)
    actual={str(p.relative_to(unpack/'PrinceOfPersia2-scaffold')):p.read_bytes()
            for p in (unpack/'PrinceOfPersia2-scaffold').rglob('*') if p.is_file()}
    assert actual==files,'archive member/content mismatch'
    # Fail if scratch, original data or emulation artifacts became tracked.
    tracked=subprocess.check_output(['git','ls-files'],cwd=ROOT,text=True).splitlines()
    assert not any(p.startswith(('tmp/','ref/','build/','dist/','amiga/obj/','amiga/out/','amiga/.run','.replaced/')) for p in tracked)
    ignored=subprocess.check_output(['git','status','--ignored','--short'],cwd=ROOT,text=True)
    assert '!! tmp/' in ignored and '!! amiga/out/' in ignored and '!! build/' in ignored
    report=dict(result='SCAFFOLD GATE PASS',positive=str(positive.relative_to(ROOT)),repeat=str(restored.relative_to(ROOT)),failed_exit=str(failed.relative_to(ROOT)),timeout=str(timed.relative_to(ROOT)),archive_sha256=hashlib.sha256(first).hexdigest(),controls=6)
    (logs/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print('SCAFFOLD GATE PASS: two positive runs, failed DOS exit, timeout, four evidence controls, deterministic independently decoded archive; outputs ignored')
    print(logs.relative_to(ROOT))
if __name__=='__main__':main()
