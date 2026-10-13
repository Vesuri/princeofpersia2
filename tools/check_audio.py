"""Item 10 gate: original service pairs, DMA data, callbacks, and rejection controls."""
import argparse,hashlib,json,os,re,shutil,struct,subprocess,tempfile
from pathlib import Path
from extract_original_data import ROOT
from audio_replay import generate,calls
from audio_events import replay
from check_audio_result import check,startup
from check_core_result import restored
from run_reference import run as reference

FLAGS='-DPOP2_CORE -DPOP2_DISPLAY -DPOP2_INPUT -DPOP2_UI -DPOP2_AUDIO'
def evidence_controls(run,logs):
    target=logs/'evidence';target.mkdir()
    names=['gdb-out.log','audio-events.bin','audio-dma.bin','logs/debug.uae','dh1/Shell.done','dh1/audio-replay.json']
    for name in names:
        path=target/name;path.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(run/name,path)
    changes=[('gdb-out.log',lambda b:b.replace(b'AUDIO / FIXTURE COMPLETE',b'AUDIO / INCOMPLETE')),
             ('gdb-out.log',lambda b:b.replace(b'heap=1',b'heap=0')),
             ('gdb-out.log',lambda b:b.replace(b'AUDIO ADK saved=0 actual=0',b'AUDIO ADK saved=0 actual=1')),
             ('logs/debug.uae',lambda b:b.replace(b'sound_output=exact',b'sound_output=none')),
             ('audio-events.bin',lambda b:bytes([b[0]^1])+b[1:]),
             ('audio-events.bin',lambda b:b[:-24]),
             ('audio-dma.bin',lambda b:b[:8]+bytes([b[8]^1])+b[9:]),
             ('audio-dma.bin',lambda b:b[:32]+bytes([b[32]^1])+b[33:]),
             ('audio-dma.bin',lambda b:b[:40]+bytes(4)+b[44:]),
             ('dh1/Shell.done',lambda b:b'SHELL PASS\n')]
    check(target)
    for name,change in changes:
        p=target/name;good=p.read_bytes();bad=change(good);assert bad!=good
        p.write_bytes(bad)
        try:check(target)
        except (AssertionError,ValueError,RuntimeError):pass
        else:raise AssertionError('AUDIO / ACCEPTED BAD EVIDENCE '+name)
        finally:p.write_bytes(good)
    return len(changes)

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--references',nargs=3,type=Path,metavar=('ROUTE','CUTSCENE','ALLOCATION'))
    args=parser.parse_args();os.chdir(ROOT)
    for key in list(os.environ):
        if key.startswith(('POP2_','CORE_','UI_','INPUT_','DISPLAY_','AUDIO_')) or key in ('CORE','GDBSCRIPT'):os.environ.pop(key)
    logs=Path(tempfile.mkdtemp(prefix='audio-gate-',dir=ROOT/'tmp'));native={}
    if args.references:route,cutscene,fixture=[p.resolve() for p in args.references]
    else:
        os.environ.update(POP2_AUDIO='route',POP2_DISPLAY_ROUTE='1');route=reference()
        os.environ['POP2_AUDIO']='cutscene';cutscene=reference()
        os.environ.pop('POP2_DISPLAY_ROUTE');os.environ['POP2_AUDIO']='fixture';fixture=reference()
        os.environ.pop('POP2_AUDIO')
    from audio_contract import verify as verify_contract
    verify_contract(fixture)
    for ref in (route,cutscene):replay(ref)
    assert {r['phase'] for r in calls(route)}=={0,1,2,3},'AUDIO / ROUTE PHASES'
    assert 'AUDIO_CUTSCENE_COMPLETE' in (cutscene/'session.log').read_text()
    subprocess.run(['c++','-std=c++17','-g','-fsanitize=address,undefined','-Isrc','tools/probe_audio_contract.cpp','-o',str(logs/'contract')],check=True)
    subprocess.run([str(logs/'contract'),str(fixture)],check=True)
    def build(extra=''):
        with (logs/'build.log').open('a') as out:subprocess.run(['make','-C','amiga','VERIFY=1','EXTRA_DEFINES='+FLAGS+extra],stdout=out,stderr=subprocess.STDOUT,check=True)
    def launch(label,success=True,**changes):
        env=dict(os.environ,CORE='1',AUDIO_PROBE='1',SOUND='1',DEADLINE='300',GDBSCRIPT=str(ROOT/'amiga/audio.gdb'));env.update(changes)
        with (logs/(label+'.log')).open('w') as out:result=subprocess.run(['/bin/bash','amiga/run.sh'],env=env,stdout=out,stderr=subprocess.STDOUT)
        run=ROOT/'amiga'/(ROOT/'amiga/.run-last.txt').read_text().strip();native[label]=run
        assert (result.returncode==0)==success,(label,logs)
        return run
    for name,ref in [('cutscene',cutscene),('route',route),('repeat',route)]:
        generate(ref);build(' -DPOP2_AUDIO_PROBE');check(launch(name))
    for name in ('audio-events.bin','audio-dma.bin'):
        assert (native['route']/name).read_bytes()==(native['repeat']/name).read_bytes(),'AUDIO / NATIVE REPEAT'
    controls=evidence_controls(native['repeat'],logs)
    for name in ('bad-audio-pitch','bad-audio-replay'):
        run=launch(name,False,CORE_CONTROL=name);log=(run/'gdb-out.log').read_text()
        assert 'CORE REASON CORE INPUT / SHA256\n' in log and (run/'dh1/Shell.done').read_text().strip()=='SHELL FAIL'
    for label,option,reason in [('selector','SELECTOR','AUDIO / UNKNOWN SELECTOR'),('callback','CALLBACK','AUDIO / CALLBACK OWNER'),('thunk','THUNK','AUDIO / JNTH BYTES'),('resource','RESOURCE','AUDIO / ORIGINAL MDRV FORBIDDEN')]:
        build(' -DPOP2_AUDIO_PROBE -DPOP2_AUDIO_BAD_'+option);run=launch(label,False);log=(run/'gdb-out.log').read_text()
        assert 'CORE REASON '+reason+'\n' in log,(label,logs);restored(run,log)
    # Original observer guard must reject a deliberately wrong expected byte.
    os.environ['POP2_AUDIO']='route'
    reference(control='bad-audio-guard');os.environ.pop('POP2_AUDIO')
    build(' -DPOP2_STARTUP_CHECKPOINT')
    run=launch('startup',True,AUDIO_PROBE='0',AUDIO_STARTUP='1',AUDIO_REFERENCE=str(route),GDBSCRIPT=str(ROOT/'amiga/core.gdb'))
    startup(run,route)
    ignored=subprocess.check_output(['git','status','--ignored','--short'],text=True)
    assert all('!! '+p+'/' in ignored for p in ('tmp','ref','build'))
    tracked=subprocess.check_output(['git','ls-files'],text=True).splitlines()
    assert not any(p.startswith(('tmp/','ref/','build/','amiga/obj/','amiga/out/','amiga/.run')) for p in tracked)
    result={'result':'AUDIO GATE PASS','scope':'Paired CPU service replay and Paula DMA; explicit 144-call startup regression checkpoint.',
            'references':[str(p.relative_to(ROOT)) for p in (route,cutscene,fixture)],'native':{k:str(v.relative_to(ROOT)) for k,v in native.items()},'evidence_controls':controls,'runtime_controls':6}
    (logs/'report.json').write_text(json.dumps(result,indent=2)+'\n');print('AUDIO GATE PASS:',logs.relative_to(ROOT))
if __name__=='__main__':main()
