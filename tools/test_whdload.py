"""Isolated release WHDLoad observation; a timeout dump is not a gameplay pass.

Staging/CLI protocol adapted from Vette! and Alone in the Dark. The shared
launcher owns only this fixture's recorded emulator PID.
"""
import argparse,json,os,shutil,subprocess,tempfile,time
from pathlib import Path
from build_receipt import verify
ROOT=Path(__file__).resolve().parents[1]

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--setup',choices=['a1200-030','a1200-040'],default='a1200-030')
    p.add_argument('--no-preload',action='store_true')
    p.add_argument('--mmu',action='store_true')
    p.add_argument('--seconds',type=int,default=180)
    p.add_argument('--slave',type=Path,default=ROOT/'build/whdload/PrinceOfPersia2.slave')
    p.add_argument('--ticks',type=int,default=1500)
    a=p.parse_args()
    receipt=verify()
    assert set(receipt['flags'].split())=={'-DPOP2_CORE','-DPOP2_DISPLAY','-DPOP2_INPUT','-DPOP2_UI','-DPOP2_AUDIO'}
    base=Path(tempfile.mkdtemp(prefix='whdload-release-',dir=ROOT/'tmp'))
    print(base,flush=True)
    boot=base/'boot';game=base/'game'
    for d in [boot/'s',boot/'devs/Kickstarts',game,base/'state']:d.mkdir(parents=True,exist_ok=True)
    shutil.copyfile(ROOT/'amiga/out/PrinceOfPersia2.exe',game/'PrinceOfPersia2')
    shutil.copyfile(a.slave,game/'PrinceOfPersia2.slave')
    subprocess.run(['python3',str(ROOT/'tools/stage_core.py'),str(game)],check=True)
    shutil.copyfile(os.environ['WHDLOAD_BINARY'],game/'WHDLoad')
    rom=Path(os.environ['KICKSTART_A1200']);rtb=Path(str(rom)+'.RTB')
    for src in [rom,rtb]:shutil.copyfile(src,boot/'devs/Kickstarts'/src.name)
    options=('' if a.no_preload else 'PRELOAD ')+('MMU ' if a.mmu else '')
    (boot/'s/WHDLoad.prefs').write_text('Expert\nReadDelay=0\n')
    (boot/'s/startup-sequence').write_text('DF0:C/Assign C: DF0:C\nDF0:C/Assign LIBS: DF0:Libs\nDF0:C/Assign DEVS: DH0:devs\nStack 16384\nFailAt 999\nCD DH1:\n'+f'WHDLoad PrinceOfPersia2.slave {options}SPLASHDELAY=0 NOREQ COREDUMP TIMEOUT={a.ticks} >DH0:result\n'+'Echo returned >DH0:returned\n')
    (base/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    script=base/'launch.sh'
    script.write_text('''#!/bin/bash
set -euo pipefail
cd "$1"
. "$2/amiga/env.sh"
FSUAE_RUN=.
. "$FSUAE_COMMON"
trap 'fsuae_stop' EXIT
FSUAE="$FSUAE_MMU"
AMIGA_SETUP="$3" DEBUG=0 SOUND=0 WINDOW=none WARP=1 NTSC=0
fsuae_options
fsuae_launch --hard_drive_0="$PWD/boot" --hard_drive_0_priority=10 --hard_drive_1="$PWD/game" --floppy_drive_0="$WORKBENCH_ADF" --floppy_drive_volume=0 --floppy_drive_volume_empty=0 --state_dir="$PWD/state" --logs_dir="$PWD/logs" --joystick_port_0=nothing --joystick_port_1=nothing
wait "$FSUAE_PID"
''')
    try:
        with (base/'emulator.log').open('w') as log:
            emu=subprocess.Popen(['/bin/bash',str(script),str(base),str(ROOT),a.setup],stdout=log,stderr=log)
            deadline=time.monotonic()+a.seconds
            while time.monotonic()<deadline and not (boot/'returned').exists():
                if emu.poll() is not None:raise RuntimeError('Emulator exited before shell completion')
                time.sleep(.25)
        assert (boot/'returned').exists(),'No positive shell-return marker'
        print((boot/'result').read_text(errors='replace'))
        report=game/'.whdl_register'
        assert report.exists(),'No WHDLoad report'
        print(report.read_text(encoding='latin1')[:4500])
        print('OBSERVATION COMPLETE; no gameplay acceptance claim')
    finally:
        subprocess.run(['/bin/bash','-c','. "$1/amiga/env.sh"; FSUAE_RUN="$2"; . "$FSUAE_COMMON"; fsuae_stop_previous','stop',str(ROOT),str(base)],check=True)
        if 'emu' in locals():emu.wait(timeout=10)
if __name__=='__main__':main()
