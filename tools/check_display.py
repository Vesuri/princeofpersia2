"""Reproduce item 8's modular display acceptance, with positive and failure controls."""
import argparse,json,os,re,shutil,subprocess,tempfile
from pathlib import Path
from extract_original_data import ROOT
from core_inputs import generate as core_inputs
from video_transfer import generate as video_inputs
from display_frames import generate as scene_inputs,frames
from run_reference import run as reference_run
from check_display_scenes import check as scenes_check
from check_display_probe import check as pattern_check
from check_display_services import check as services_check
from check_display_startup import check as startup_check
from check_core_result import restored


def evidence_controls(run,reference,logs):
    fixture=logs/'evidence';fixture.mkdir()
    for name in ['gdb-out.log','logs/debug.uae','dh1/Shell.done',*[str(p.relative_to(run)) for p in run.glob('scene-*.bin')]]:
        target=fixture/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(run/name,target)
    scenes_check(fixture,reference)
    mutations=[('gdb-out.log',lambda b:b''),
               ('gdb-out.log',lambda b:b.replace(b'DISPLAY_ODD id=2',b'MISSING_ODD id=2')),
               ('gdb-out.log',lambda b:b.replace(b'lof=0',b'lof=1')),
               ('gdb-out.log',lambda b:b.replace(b'heap=1',b'heap=0')),
               ('dh1/Shell.done',lambda b:b'SHELL PASS\n'),
               ('scene-1-planes.bin',lambda b:bytes([b[0]^1])+b[1:]),
               ('scene-2-logical.bin',lambda b:b[:30785]+bytes([b[30785]^1])+b[30786:]),
               ('scene-1-copper.bin',lambda b:b[:123]+bytes([b[123]^1])+b[124:]),
               ('scene-1-odd-copper.bin',lambda b:b[:6]+bytes([b[6]^1])+b[7:]),
               ('scene-2-copper.bin',lambda b:b[:67]+bytes([b[67]^1])+b[68:])]
    for name,change in mutations:
        path=fixture/name;good=path.read_bytes();path.write_bytes(change(good))
        try:scenes_check(fixture,reference)
        except (AssertionError,RuntimeError,ValueError):pass
        else:raise AssertionError('DISPLAY / CONTROL ACCEPTED '+name)
        finally:path.write_bytes(good)
    scenes_check(fixture,reference)
    return len(mutations)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--references',nargs=4,type=Path,metavar=('SCENES','REPEAT','SERVICES','VIDEO'))
    args=parser.parse_args();os.chdir(ROOT)
    logs=Path(tempfile.mkdtemp(prefix='display-gate-',dir=ROOT/'tmp'))
    for key in list(os.environ):
        if key.startswith(('POP2_','CORE_','DISPLAY_')) or key in ('CORE','GDBSCRIPT'):os.environ.pop(key)
    if args.references:scenes,repeat,services,video=[p.resolve() for p in args.references]
    else:
        os.environ['POP2_DISPLAY_ROUTE']='1';scenes=reference_run();repeat=reference_run();os.environ.pop('POP2_DISPLAY_ROUTE')
        os.environ['POP2_DISPLAY']='1';services=reference_run();os.environ.pop('POP2_DISPLAY')
        os.environ['POP2_VIDEO_TRANSFER']='1';video=reference_run();os.environ.pop('POP2_VIDEO_TRANSFER')
    video_inputs(video);scene_inputs(scenes);assert frames(scenes)==frames(repeat)
    for tool in ('check_copybits.py','check_animate_palette.py'):
        subprocess.run(['python3','tools/'+tool,str(services)],check=True)
    core_inputs();native={}
    env=dict(os.environ,CORE='1',DISPLAY_REFERENCE=str(scenes))
    def build(flags):
        with (logs/('build-'+flags.replace(' ','_')+'.log')).open('w') as stream:
            subprocess.run(['make','-C','amiga','VERIFY=1','EXTRA_DEFINES='+flags],stdout=stream,stderr=subprocess.STDOUT,check=True)
    def launch(label,success=True,**changes):
        with (logs/(label+'.log')).open('w') as stream:
            result=subprocess.run(['/bin/bash','amiga/run.sh'],env=dict(env,**changes),stdout=stream,stderr=subprocess.STDOUT)
        path=ROOT/'amiga'/(ROOT/'amiga/.run-last.txt').read_text().strip();native[label]=path
        assert result.returncode==0 if success else result.returncode!=0,(label,logs)
        return path
    build('-DPOP2_DISPLAY_PROBE')
    pattern_check(launch('pattern',CORE='0',GDBSCRIPT=str(ROOT/'amiga/display_probe.gdb')))
    build('-DPOP2_CORE -DPOP2_DISPLAY -DPOP2_DISPLAY_SERVICES_PROBE')
    services_check(launch('services',DISPLAY_SERVICES='1',DISPLAY_REFERENCE=str(services),GDBSCRIPT=str(ROOT/'amiga/display_services.gdb')),services)
    build('-DPOP2_CORE -DPOP2_DISPLAY -DPOP2_DISPLAY_FRAMES_PROBE')
    for name in ('scenes','repeat'):
        scenes_check(launch(name,DISPLAY_SCENES='1',GDBSCRIPT=str(ROOT/'amiga/display_scenes.gdb')),scenes)
    for suffix in ('planes','logical'):
        for n in (1,2):assert (native['scenes']/f'scene-{n}-{suffix}.bin').read_bytes()==(native['repeat']/f'scene-{n}-{suffix}.bin').read_bytes()
    controls=evidence_controls(native['repeat'],scenes,logs)
    for control in ('bad-transfer','bad-frames'):
        path=launch(control,False,CORE_CONTROL=control,DISPLAY_STARTUP='1')
        assert 'CORE INPUT / SHA256' in (path/'gdb-out.log').read_text(),control
        assert (path/'dh1/Shell.done').read_text().strip()=='SHELL FAIL'
    build('-DPOP2_CORE -DPOP2_DISPLAY -DPOP2_DISPLAY_SERVICES_PROBE -DPOP2_DISPLAY_BAD_SELECTOR')
    path=launch('unknown-selector',False,DISPLAY_STARTUP='1')
    log=(path/'gdb-out.log').read_text();restored(path,log)
    assert 'CORE REASON DISPLAY / QDEXTENSIONS SELECTOR\n' in log and 'CORE STOP trap=ab1d ' in log
    build('-DPOP2_CORE -DPOP2_DISPLAY')
    startup_check(launch('startup',DISPLAY_STARTUP='1'),scenes)
    tracked=subprocess.check_output(['git','ls-files'],text=True).splitlines()
    assert not any(p.startswith(('tmp/','ref/','build/','amiga/obj/','amiga/out/','amiga/.run')) for p in tracked)
    ignored=subprocess.check_output(['git','status','--ignored','--short'],text=True)
    assert all('!! '+p+'/' in ignored for p in ('tmp','ref','build'))
    result={'result':'DISPLAY GATE PASS','scope':'paired services and captured-scene presentation; original startup ends at InitFonts',
            'reference':[str(p.relative_to(ROOT)) for p in (scenes,repeat,services,video)],
            'native':{k:str(v.relative_to(ROOT)) for k,v in native.items()},'evidence_controls':controls,'native_controls':3}
    (logs/'report.json').write_text(json.dumps(result,indent=2)+'\n')
    print('DISPLAY GATE PASS:',logs.relative_to(ROOT),flush=True)

if __name__=='__main__':main()
