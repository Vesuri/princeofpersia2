"""Run a pinned wrapper, SaveRect or C2P fixture in an isolated native machine."""
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


def run(fixture, seconds, kind="wrapper"):
    assert kind in ("wrapper", "save-rect", "c2p")
    c2p=kind=="c2p"
    raster=kind=="save-rect"
    fixture=fixture.resolve()
    for group in ('build','run'):verify(group)
    stub=ROOT/('tools/c2p_abi.s' if c2p else 'tools/wrapper_ccr_fixture.s')
    if c2p:
        verify('calibrated')
        for name,digest in pins(ROOT/'manifest/kalms-c2p.sha256').items():
            assert hashlib.sha256((ROOT/name).read_bytes()).hexdigest()==digest,name
        for name,digest in pins(ROOT/'manifest/kalms-ndk.sha256').items():
            assert hashlib.sha256((Path(os.environ['NDK_ASM'])/name).read_bytes()).hexdigest()==digest,name
        meta={'image_sha256':hashlib.sha256((ROOT/'src/platform/amiga/kalms/c2p1x1_8_c5_030_2.s').read_bytes()).hexdigest()}
    elif raster:
        from save_rect_fixture import verify_fixture
        verify_fixture(fixture)
        verify('calibrated')
        meta={'image_sha256':hashlib.sha256((fixture/'original.bin').read_bytes()).hexdigest()}
    else:
        recorded=pins(ROOT/'manifest/wrapper-ccr-inputs.sha256')
        for name in ('image.bin','metadata.json','metadata.lua','fixture.h'):
            p=fixture/name;key=str(p.relative_to(ROOT))
            assert key in recorded and hashlib.sha256(p.read_bytes()).hexdigest()==recorded[key],key
        meta=json.loads((fixture/'metadata.json').read_text())
        assert hashlib.sha256(stub.read_bytes()).hexdigest()==meta['stub_sha256']
    source=ROOT/('tools/c2p_native.c' if c2p else 'tools/save_rect_native.c' if raster else 'tools/wrapper_ccr_native.c')
    base=Path(tempfile.mkdtemp(prefix=kind+'-native-',dir=ROOT/'tmp'))
    print(base.relative_to(ROOT),flush=True)
    for name in ('dh0/s','dh0/c','dh1','state','logs'):(base/name).mkdir(parents=True,exist_ok=True)
    shutil.copyfile(os.environ['SETPATCH'],base/'dh0/c/SetPatch')
    subprocess.run(['m68k-amiga-elf-as','-mcpu=68020','--register-prefix-optional',str(stub),'-o',str(base/'bridge.o')],check=True)
    objects=[str(base/'bridge.o')]
    if raster:
        subprocess.run(['m68k-amiga-elf-as','-mcpu=68020','--register-prefix-optional','src/platform/amiga/SaveRect.s','-o',str(base/'raster.o')],cwd=ROOT,check=True)
        objects.append(str(base/'raster.o'))
    if c2p:
        subprocess.run([os.environ['VASM'],'-m68020','-Felf','-quiet','-x','-no-opt','-nowarn=62','-I'+os.environ['NDK_ASM'],'../src/platform/amiga/DisplayC2P.s','-o',str(base/'c2p.o')],cwd=ROOT/'amiga',check=True)
        objects.append(str(base/'c2p.o'))
    elf=base/'fixture.elf';exe=base/'dh1/WrapperProbe'
    subprocess.run(['m68k-amiga-elf-gcc','-O2','-m68020','-nostdlib','-fno-tree-loop-distribute-patterns',
        '-ffunction-sections','-fdata-sections','-Wno-volatile-register-var','-I'+str(fixture),
        '-Wl,--gc-sections,--emit-relocs,-Ttext=0,-e,_start',str(ROOT/'tools/install-data/start.s'),
        str(source),*objects,'-o',str(elf)],check=True)
    subprocess.run([os.environ['ELF2HUNK'],str(elf),str(exe),'-s'],check=True)
    receipt={'fixture':str(fixture.relative_to(ROOT)),'image':meta['image_sha256'],
             'exe':hashlib.sha256(exe.read_bytes()).hexdigest(),'elf':hashlib.sha256(elf.read_bytes()).hexdigest(),
             'native_source':hashlib.sha256(source.read_bytes()).hexdigest()}
    if raster:
        receipt['sources']={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in ('tools/wrapper_ccr_fixture.s','src/platform/amiga/SaveRect.s','src/mac/ForwardLongCopy.inc')}
    if c2p:
        receipt['sources']={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in ('tools/c2p_abi.s','src/platform/amiga/DisplayC2P.s','src/platform/amiga/kalms/KalmsC2PRect.s','src/platform/amiga/kalms/c2p1x1_8_c5_030_2.s')}
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
FSUAE="${FIXTURE_EMULATOR:-$FSUAE_ARM}"
AMIGA_SETUP="${FIXTURE_SETUP:-a1200-030}" DEBUG=0 SOUND=0 WINDOW=none WARP=1 NTSC=0
fsuae_options
fsuae_launch --hard_drive_0="$PWD/dh0" --hard_drive_1="$PWD/dh1" --logs_dir="$PWD/logs" --state_dir="$PWD/state" --floppy_drive_volume=0 --floppy_drive_volume_empty=0
wait "$FSUAE_PID"
''')
    process=None
    try:
        with (base/'emulator.log').open('w') as log:
            process=subprocess.Popen(['/bin/bash',str(launcher),str(base),str(ROOT)],stdout=log,stderr=log,env=dict(os.environ,**({'FIXTURE_EMULATOR':os.environ['FSUAE_CALIBRATED'],'FIXTURE_SETUP':'a1200-fast'} if (raster or c2p) else {})))
            deadline=time.monotonic()+seconds
            while time.monotonic()<deadline and not (base/'dh1/Shell.done').exists():
                if process.poll() is not None:raise RuntimeError('WRAPPER / EMULATOR EXIT')
                time.sleep(.5)
        assert (base/'dh1/Shell.done').read_text()=='SHELL PASS\n','WRAPPER / SHELL COMPLETION'
        if c2p:
            from native_machine import check as check_machine
            check_machine(base,'a1200-fast')
            assert (base/'dh1/fixture.complete').read_text()=='C2P_COMPLETE cases=193\n','C2P / FIXTURE COMPLETION'
        elif raster:
            assert (base/'dh1/fixture.complete').read_text()=='SAVE_RECT_COMPLETE\n','SAVE RECT / FIXTURE COMPLETION'
            assert (base/'dh1/original.bin').read_bytes()==(base/'dh1/replacement.bin').read_bytes(),'SAVE RECT / NATIVE DIFFERENTIAL'
        else:
            assert (base/'dh1/fixture.complete').read_text()=='WRAPPER_FIXTURE_COMPLETE cases=640\n','WRAPPER / FIXTURE COMPLETION'
            assert (base/'dh1/wrapper-rows.bin').stat().st_size==640*23*4,'WRAPPER / ROW EXTENT'
        print(kind.upper()+' NATIVE COMPLETE:',base.relative_to(ROOT),flush=True)
    finally:
        subprocess.run(['/bin/bash','-c','. "$1/amiga/env.sh"; FSUAE_RUN="$2"; . "$FSUAE_COMMON"; fsuae_stop_previous','stop',str(ROOT),str(base)],check=True)
        if process is not None:process.wait(timeout=10)
    return base


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('fixture',type=Path)
    p.add_argument('--kind',choices=['wrapper','save-rect','c2p'],default='wrapper')
    p.add_argument('--seconds',type=int,default=300);a=p.parse_args()
    if not 1<=a.seconds<=3600:p.error('--seconds must be between 1 and 3600')
    run(a.fixture,a.seconds,a.kind)
