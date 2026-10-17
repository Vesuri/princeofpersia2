"""Run the pinned wrapper ABI image in an isolated a1200-030 native fixture."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import time
from extract_original_data import ROOT, pins
from scaffold_inputs import verify


def run(fixture, seconds):
    fixture=fixture.resolve()
    for group in ('build','run'):verify(group)
    recorded=pins(ROOT/'manifest/wrapper-ccr-inputs.sha256')
    for name in ('image.bin','metadata.json','metadata.lua','fixture.h'):
        p=fixture/name;key=str(p.relative_to(ROOT))
        assert key in recorded and hashlib.sha256(p.read_bytes()).hexdigest()==recorded[key],key
    meta=json.loads((fixture/'metadata.json').read_text())
    stub=ROOT/'tools/wrapper_ccr_fixture.s'
    assert hashlib.sha256(stub.read_bytes()).hexdigest()==meta['stub_sha256']
    base=Path(tempfile.mkdtemp(prefix='wrapper-native-',dir=ROOT/'tmp'))
    print(base.relative_to(ROOT),flush=True)
    for name in ('dh0/s','dh0/c','dh1','state','logs'):(base/name).mkdir(parents=True,exist_ok=True)
    shutil.copyfile(os.environ['SETPATCH'],base/'dh0/c/SetPatch')
    subprocess.run(['m68k-amiga-elf-as','-mcpu=68020','--register-prefix-optional',str(stub),'-o',str(base/'bridge.o')],check=True)
    elf=base/'fixture.elf';exe=base/'dh1/WrapperProbe'
    subprocess.run(['m68k-amiga-elf-gcc','-O2','-m68020','-nostdlib','-fno-tree-loop-distribute-patterns',
        '-ffunction-sections','-fdata-sections','-Wno-volatile-register-var','-I'+str(fixture),
        '-Wl,--gc-sections,--emit-relocs,-Ttext=0,-e,_start',str(ROOT/'tools/install-data/start.s'),
        str(ROOT/'tools/wrapper_ccr_native.c'),str(base/'bridge.o'),'-o',str(elf)],check=True)
    subprocess.run([os.environ['ELF2HUNK'],str(elf),str(exe),'-s'],check=True)
    receipt={'fixture':str(fixture.relative_to(ROOT)),'image':meta['image_sha256'],
             'exe':hashlib.sha256(exe.read_bytes()).hexdigest(),'elf':hashlib.sha256(elf.read_bytes()).hexdigest(),
             'native_source':hashlib.sha256((ROOT/'tools/wrapper_ccr_native.c').read_bytes()).hexdigest()}
    (base/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    (base/'dh0/s/startup-sequence').write_text('C:SetPatch QUIET\nFailAt 21\nStack 16384\ncd dh1:\nWrapperProbe\nIf WARN\nEcho "SHELL FAIL" >Shell.done\nElse\nEcho "SHELL PASS" >Shell.done\nEndIf\n')
    launcher=base/'launch.sh'
    launcher.write_text('''#!/bin/bash
set -euo pipefail
cd "$1"
. "$2/amiga/env.sh"
FSUAE_RUN=.
. "$FSUAE_COMMON"
trap 'fsuae_stop' EXIT
FSUAE="$FSUAE_ARM"
AMIGA_SETUP=a1200-030 DEBUG=0 SOUND=0 WINDOW=none WARP=1 NTSC=0
fsuae_options
fsuae_launch --hard_drive_0="$PWD/dh0" --hard_drive_1="$PWD/dh1" --logs_dir="$PWD/logs" --state_dir="$PWD/state" --floppy_drive_volume=0 --floppy_drive_volume_empty=0
wait "$FSUAE_PID"
''')
    process=None
    try:
        with (base/'emulator.log').open('w') as log:
            process=subprocess.Popen(['/bin/bash',str(launcher),str(base),str(ROOT)],stdout=log,stderr=log)
            deadline=time.monotonic()+seconds
            while time.monotonic()<deadline and not (base/'dh1/Shell.done').exists():
                if process.poll() is not None:raise RuntimeError('WRAPPER / EMULATOR EXIT')
                time.sleep(.5)
        assert (base/'dh1/Shell.done').read_text()=='SHELL PASS\n','WRAPPER / SHELL COMPLETION'
        assert (base/'dh1/fixture.complete').read_text()=='WRAPPER_FIXTURE_COMPLETE cases=640\n','WRAPPER / FIXTURE COMPLETION'
        assert (base/'dh1/wrapper-rows.bin').stat().st_size==640*23*4,'WRAPPER / ROW EXTENT'
        print('WRAPPER NATIVE COMPLETE:',base.relative_to(ROOT),flush=True)
    finally:
        subprocess.run(['/bin/bash','-c','. "$1/amiga/env.sh"; FSUAE_RUN="$2"; . "$FSUAE_COMMON"; fsuae_stop_previous','stop',str(ROOT),str(base)],check=True)
        if process is not None:process.wait(timeout=10)
    return base


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('fixture',type=Path)
    p.add_argument('--seconds',type=int,default=300);a=p.parse_args()
    if not 1<=a.seconds<=3600:p.error('--seconds must be between 1 and 3600')
    run(a.fixture,a.seconds)
