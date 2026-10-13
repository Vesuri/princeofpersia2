"""Item 9 gate: paired original input/UI contracts, AGA output and loud stops."""
import argparse,hashlib,json,os,re,shutil,subprocess,tempfile
from pathlib import Path
from extract_original_data import ROOT,pins
from run_reference import run as reference_run
from check_ui_result import check,check_input,startup,reference,input_reference
from check_core_result import restored
from ui_inputs import generate


def evidence_controls(run,ref,logs):
    fixture=logs/'evidence';fixture.mkdir()
    names=['gdb-out.log','logs/debug.uae','dh1/Shell.done',*[p.name for p in run.glob('ui-*.bin')]]
    for name in names:
        target=fixture/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(run/name,target)
    check(fixture,ref)
    changes=[('gdb-out.log',lambda b:b''),
             ('gdb-out.log',lambda b:b.replace(b'UI / FIXTURE COMPLETE',b'UI / INCOMPLETE')),
             ('gdb-out.log',lambda b:b.replace(b'INPUT RESTORE vector=1',b'INPUT RESTORE vector=0')),
             ('gdb-out.log',lambda b:b.replace(b'heap=1',b'heap=0')),
             ('gdb-out.log',lambda b:b.replace(b'UI RESULT index=20 value=00000002',b'UI RESULT index=20 value=00000001')),
             ('gdb-out.log',lambda b:re.sub(rb'(UI OVERLAY[^\n]+dma=)([0-9a-f]+)',lambda m:m[1]+format(int(m[2],16)|0x20,'04x').encode(),b)),
             ('dh1/Shell.done',lambda b:b'SHELL PASS\n'),
             ('ui-keymaps.bin',lambda b:b'\x01'+b[1:]),
             ('ui-2-planes.bin',lambda b:bytes([b[0]^1])+b[1:]),
             ('ui-3-logical.bin',lambda b:b'\x00'+b[1:]),
             ('ui-4-copper.bin',lambda b:b[:123]+bytes([b[123]^1])+b[124:]),
             ('ui-window-planes.bin',lambda b:b'\x00'+b[1:])]
    for name,change in changes:
        path=fixture/name;good=path.read_bytes();bad=change(good);assert bad!=good,name
        path.write_bytes(bad)
        try:check(fixture,ref)
        except (AssertionError,RuntimeError,ValueError):pass
        else:raise AssertionError('UI / ACCEPTED BAD EVIDENCE '+name)
        finally:path.write_bytes(good)
    check(fixture,ref)
    return len(changes)


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--references',nargs=4,type=Path,metavar=('UI','UI_REPEAT','INPUT','INPUT_REPEAT'))
    a=p.parse_args();os.chdir(ROOT)
    logs=Path(tempfile.mkdtemp(prefix='ui-gate-',dir=ROOT/'tmp'))
    for key in list(os.environ):
        if key.startswith(('POP2_','CORE_','UI_','INPUT_','DISPLAY_')) or key in ('CORE','GDBSCRIPT'):os.environ.pop(key)
    if a.references:ui,repeat,keys,keys_repeat=[v.resolve() for v in a.references]
    else:
        os.environ['POP2_UI_ROUTINES']='1';ui=reference_run();repeat=reference_run();os.environ.pop('POP2_UI_ROUTINES')
        os.environ['POP2_INPUT_PROBE']='1';keys=reference_run();keys_repeat=reference_run();os.environ.pop('POP2_INPUT_PROBE')
    assert reference(ui)==reference(repeat),'UI / ORIGINAL REPEAT'
    assert input_reference(keys)==input_reference(keys_repeat),'INPUT / ORIGINAL REPEAT'
    semantic={'ui':reference(ui),'input':(input_reference(keys)[0],sorted(k.hex() for k in input_reference(keys)[1]))}
    digest=hashlib.sha256((json.dumps(semantic,sort_keys=True,separators=(',',':'))+'\n').encode()).hexdigest()
    assert digest==pins(ROOT/'manifest/ui-reference.sha256')['ui-contracts.json'],'UI / ORIGINAL CONTRACT PIN'
    # Sanitizers cover the portable queue and original physical-key contract.
    for source,args in [('test_input_state.cpp',[]),('probe_input_reference.cpp',[str(keys/'input-samples.bin')])]:
        exe=logs/source.removesuffix('.cpp')
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-fsanitize=address,undefined','-g','-Isrc','tools/'+source,'-o',str(exe)],check=True)
        subprocess.run([str(exe),*args],check=True)
    generate();native={};flags='-DPOP2_CORE -DPOP2_DISPLAY -DPOP2_INPUT -DPOP2_UI'
    env=dict(os.environ,CORE='1',UI_REFERENCE=str(ui),INPUT_REFERENCE=str(keys),DEADLINE='300')
    def build(options):
        with (logs/('build-'+options.replace(' ','_')+'.log')).open('w') as out:
            subprocess.run(['make','-C','amiga','EXTRA_DEFINES='+options],stdout=out,stderr=subprocess.STDOUT,check=True)
    def launch(label,success=True,**changes):
        with (logs/(label+'.log')).open('w') as out:
            result=subprocess.run(['/bin/bash','amiga/run.sh'],env=dict(env,**changes),stdout=out,stderr=subprocess.STDOUT)
        path=ROOT/'amiga'/(ROOT/'amiga/.run-last.txt').read_text().strip();native[label]=path
        assert (result.returncode==0) if success else (result.returncode!=0),(label,logs)
        return path
    build(flags+' -DPOP2_INPUT_PROBE')
    check_input(launch('input',INPUT_PROBE='1',GDBSCRIPT=str(ROOT/'amiga/input_probe.gdb')),keys)
    build(flags+' -DPOP2_UI_PROBE')
    for name in ('ui','repeat'):
        check(launch(name,UI_PROBE='1',GDBSCRIPT=str(ROOT/'amiga/ui_probe.gdb')),ui)
    for number in range(1,8):
        for suffix in ('logical','planes'):
            name=f'ui-{number}-{suffix}.bin';assert (native['ui']/name).read_bytes()==(native['repeat']/name).read_bytes(),'UI / NATIVE REPEAT'
    controls=evidence_controls(native['repeat'],ui,logs)
    # Runtime guards must fail independently of the paired-result checker.
    path=launch('bad-plan',False,UI_STARTUP='1',CORE_CONTROL='bad-ui-plan')
    assert 'CORE REASON CORE INPUT / SHA256\n' in (path/'gdb-out.log').read_text()
    assert (path/'dh1/Shell.done').read_text().strip()=='SHELL FAIL'
    for label,option,reason in [('bad-byte','-DPOP2_BAD_UI_GUARD','UI / ORIGINAL BYTE MISMATCH'),
                                ('unknown-about','-DPOP2_UI_PROBE -DPOP2_UI_BAD_ABOUT','UI / ABOUT ID'),
                                ('raw-overflow','-DPOP2_INPUT_OVERFLOW','INPUT / RAW QUEUE OVERFLOW')]:
        build(flags+' '+option);path=launch(label,False,UI_STARTUP='1')
        log=(path/'gdb-out.log').read_text();assert 'CORE REASON '+reason+'\n' in log,(label,logs)
        assert (path/'dh1/Shell.done').read_text().strip()=='SHELL FAIL'
        if label!='bad-byte':restored(path,log)
        assert 'INPUT RESTORE vector=1\n' in log
    build(flags+' -DPOP2_STARTUP_CHECKPOINT');startup(launch('startup',UI_STARTUP='1'),ui)
    tracked=subprocess.check_output(['git','ls-files'],text=True).splitlines()
    assert not any(v.startswith(('tmp/','ref/','build/','amiga/obj/','amiga/out/','amiga/.run')) for v in tracked)
    ignored=subprocess.check_output(['git','status','--ignored','--short'],text=True)
    assert all('!! '+v+'/' in ignored for v in ('tmp','ref','build'))
    result={'result':'UI GATE PASS','scope':'144 original startup calls; paired input and UI routine/service fixtures; no native gameplay claim',
            'references':[str(v.relative_to(ROOT)) for v in (ui,repeat,keys,keys_repeat)],
            'native':{k:str(v.relative_to(ROOT)) for k,v in native.items()},'evidence_controls':controls,'native_controls':4}
    (logs/'report.json').write_text(json.dumps(result,indent=2)+'\n')
    print('UI GATE PASS:',logs.relative_to(ROOT),flush=True)

if __name__=='__main__':main()
