"""Pair ordinary WHDLoad quit with the original application's cleanup path."""
import argparse
import gzip
import hashlib
import json
import re
import shutil
import tempfile
from pathlib import Path
from extract_original_data import ROOT, pins
from whdload_gameplay import check as native_check


def check(original, native):
    expected=pins(ROOT/'manifest/quit-reference.sha256')
    evidence={}
    for name,digest in expected.items():
        path=original/name
        data=path.read_bytes() if path.exists() else gzip.decompress(Path(str(path)+'.gz').read_bytes())
        assert hashlib.sha256(data).hexdigest()==digest, 'QUIT / ORIGINAL PIN '+name
        evidence[name]=data
    log=evidence['session.log'].decode()
    assert log.count('QUIT_REFERENCE_COMPLETE Finder\n')==1 and 'FAIL REFERENCE' not in log, 'QUIT / ORIGINAL COMPLETION'
    assert log.count('QUIT_AUDIO_ENTER selector=8 ')==1, 'QUIT / ORIGINAL AUDIO ENTRY'
    assert log.count('QUIT_AUDIO_RETURN result=00000000 ')==1, 'QUIT / ORIGINAL AUDIO RETURN'
    entered=re.search(r'QUIT_DISPOSE_ENTER window=([0-9A-F]+) current=([0-9A-F]+)',log)
    returned=re.search(r'QUIT_DISPOSE_RETURN count=1 current=([0-9A-F]+) disposed=([0-9A-F]+)',log)
    assert entered and returned and entered[1]==entered[2]==returned[2] and returned[1]!=returned[2], 'QUIT / ORIGINAL PORT FALLBACK'
    assert 'QUIT_EXIT_TO_SHELL count=1 ' in log, 'QUIT / ORIGINAL EXIT'
    native_check(native)
    assert (native/'gdb.log').read_text().count('QUIT AUDIO RETURN selector=8 result=0')==1, 'QUIT / PAIRED AUDIO RETURN'
    for name,size in [('quit-window-before.bin',156),('quit-port-after.bin',108)]:
        actual=(native/name).read_bytes();reference=evidence[name]
        assert len(actual)==len(reference)==size, 'QUIT / PORT EXTENT'
        assert actual[16:24]==reference[16:24], 'QUIT / PORT RECTANGLE'
    result={'result':'PAIRED RELEASE QUIT PASS','original':expected['session.log'],
            'exe':json.loads((native/'receipt.json').read_text())['exe']}
    (native/'paired-quit.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result))


def controls(original,native):
    with tempfile.TemporaryDirectory(prefix='quit-controls-',dir=ROOT/'tmp') as directory:
        base=Path(directory)
        for name in ['gdb.log','receipt.json','first-context.bin','moved-context.bin','first-a5.bin','moved-a5.bin','quit-window-before.bin','quit-port-after.bin']:
            shutil.copyfile(native/name,base/name)
        (base/'game').mkdir();shutil.copyfile(native/'game/PrinceOfPersia2',base/'game/PrinceOfPersia2')
        if (native/'game/.whdl_register').exists():shutil.copyfile(native/'game/.whdl_register',base/'game/.whdl_register')
        (base/'boot').mkdir()
        for name in ['passed','returned']:shutil.copyfile(native/'boot'/name,base/'boot'/name)
        # Preserve the scenario when testing save/load routes. Otherwise the
        # default eight-update check would reject every mutation for the wrong reason.
        options=native/'gameplay-options.json'
        if options.exists():
            shutil.copyfile(options,base/options.name)
            settings=json.loads(options.read_text())
            if settings.get('mmu_functional') or settings.get('cache_memory'):
                for name in ('logs/debug.uae','boot/s/startup-sequence'):
                    (base/name).parent.mkdir(parents=True,exist_ok=True)
                    shutil.copyfile(native/name,base/name)
            action=settings.get('save_action')
            if action:
                names=[action+'-return-a5.bin','game/Level 1','game/Level 1.rsrc','game/Level 1.finfo']
                if action=='load':names+=['save-input.json','save-input-a5.bin','loaded-first-a5.bin']
                for name in names:shutil.copyfile(native/name,base/name)
        check(original,base)  # The undamaged control must pass first.
        path=base/'gdb.log';text=path.read_text()
        for marker in ['PLAY MOVED','CORE / EXIT TO SHELL','QUIT AUDIO RETURN']:
            path.write_text(text.replace(marker,'UNREACHED'))
            try:check(original,base)
            except AssertionError:pass
            else:raise AssertionError('QUIT / MISSING-MARKER CONTROL ACCEPTED')
        path.write_text(text)
        (base/'boot/failed').touch()
        try:check(original,base)
        except AssertionError:pass
        else:raise AssertionError('QUIT / FAILED-EXIT CONTROL ACCEPTED')
        (base/'boot/failed').unlink()
        report=base/'game/.whdl_register';saved=report.read_bytes() if report.exists() else None
        report.write_text('Exception: access fault\n')
        try:check(original,base)
        except AssertionError:pass
        else:raise AssertionError('QUIT / WHDLOAD ERROR CONTROL ACCEPTED')
        if saved is None:report.unlink()
        else:report.write_bytes(saved)
        path=base/'quit-port-after.bin';data=bytearray(path.read_bytes());data[16]^=1;path.write_bytes(data)
        try:check(original,base)
        except AssertionError:pass
        else:raise AssertionError('QUIT / PORT CONTROL ACCEPTED')
    print('QUIT REJECTION CONTROLS PASS')


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('original',type=Path);p.add_argument('native',type=Path)
    p.add_argument('--controls',action='store_true');a=p.parse_args()
    check(a.original,a.native)
    if a.controls:controls(a.original,a.native)
