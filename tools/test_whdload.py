"""Isolated release WHDLoad checks; a timeout dump is not a gameplay pass.

Staging/CLI protocol adapted from Vette! and Alone in the Dark. The shared
launcher owns only this fixture's recorded emulator PID.
"""
import argparse,hashlib,json,os,shutil,subprocess,tempfile,time
from pathlib import Path
from build_receipt import verify
ROOT=Path(__file__).resolve().parents[1]

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--setup',choices=['a1200-030','a1200-040'],default='a1200-030')
    p.add_argument('--no-preload',action='store_true')
    p.add_argument('--mmu',action='store_true')
    p.add_argument('--mmu-functional',action='store_true',help='040/MMU save/load and normal-exit coverage; excludes per-trap return observations')
    p.add_argument('--seconds',type=int,default=180)
    p.add_argument('--slave',type=Path,default=ROOT/'build/whdload/PrinceOfPersia2.slave')
    p.add_argument('--ticks',type=int,default=1500)
    p.add_argument('--store-reference',type=Path,help='verify the CODE 20 store against a completed original demo trace')
    p.add_argument('--gameplay',action='store_true',help='exercise release gameplay and normal quit with guest keyboard input')
    p.add_argument('--quit-key',action='store_true',help='use WHDLoad F10 after the gameplay checkpoint')
    saves=p.add_mutually_exclusive_group()
    saves.add_argument('--save-game',action='store_true',help='save Level 1 before normal quit')
    saves.add_argument('--load-game-from',type=Path,help='load Level 1 from a completed release save fixture in a fresh boot')
    a=p.parse_args()
    assert not a.quit_key or a.gameplay, 'WHDLOAD / QUIT KEY REQUIRES GAMEPLAY'
    assert not (a.gameplay and a.store_reference), 'WHDLOAD / ONE OBSERVER'
    assert not (a.save_game or a.load_game_from) or (a.gameplay and not a.quit_key), 'WHDLOAD / SAVE REQUIRES NORMAL GAMEPLAY'
    debug=a.gameplay or bool(a.store_reference)
    assert not a.mmu_functional or (a.setup=='a1200-040' and a.mmu and a.gameplay and (a.save_game or a.load_game_from)), 'WHDLOAD / MMU FUNCTIONAL ROUTE'
    if debug:
        assert a.mmu_functional or (a.setup=='a1200-030' and not a.mmu), 'WHDLOAD / VALIDATED DEBUGGER SETUP'
        from whdload_store import emulator
        emulator()
    receipt=verify()
    assert set(receipt['flags'].split())=={'-DPOP2_CORE','-DPOP2_DISPLAY','-DPOP2_INPUT','-DPOP2_UI','-DPOP2_AUDIO'}
    base=Path(tempfile.mkdtemp(prefix='whdload-release-',dir=ROOT/'tmp'))
    print(base,flush=True)
    boot=base/'boot';game=base/'game'
    for d in [boot/'s',boot/'devs/Kickstarts',game,base/'state']:d.mkdir(parents=True,exist_ok=True)
    shutil.copyfile(ROOT/'amiga/out/PrinceOfPersia2.exe',game/'PrinceOfPersia2')
    shutil.copyfile(a.slave,game/'PrinceOfPersia2.slave')
    subprocess.run(['python3',str(ROOT/'tools/stage_core.py'),str(game)],check=True)
    if a.load_game_from:
        from whdload_gameplay import check
        source=a.load_game_from.resolve()
        check(source)
        assert json.loads((source/'gameplay-options.json').read_text()).get('save_action')=='save', 'WHDLOAD / COMPLETED SAVE REQUIRED'
        hashes={}
        for name in ('Level 1','Level 1.rsrc','Level 1.finfo'):
            assert not (game/name).exists(), 'WHDLOAD / FRESH SAVE DESTINATION'
            shutil.copyfile(source/'game'/name,game/name)
            hashes[name]=hashlib.sha256((game/name).read_bytes()).hexdigest()
        shutil.copyfile(source/'save-return-a5.bin',base/'save-input-a5.bin')
        (base/'save-input.json').write_text(json.dumps(hashes,indent=2)+'\n')
    shutil.copyfile(os.environ['WHDLOAD_BINARY'],game/'WHDLoad')
    rom=Path(os.environ['KICKSTART_A1200']);rtb=Path(str(rom)+'.RTB')
    for src in [rom,rtb]:shutil.copyfile(src,boot/'devs/Kickstarts'/src.name)
    options=('' if a.no_preload else 'PRELOAD ')+('MMU ' if a.mmu else 'NOMMU ' if debug else '')
    (base/'gameplay-options.json').write_text(json.dumps({'setup':a.setup,'mmu':a.mmu,'mmu_functional':a.mmu_functional,'preload':not a.no_preload,'quit_key':a.quit_key,'save_action':'save' if a.save_game else 'load' if a.load_game_from else None})+'\n')
    timeout='' if a.quit_key else f'TIMEOUT={a.ticks} '
    (boot/'s/WHDLoad.prefs').write_text(('' if a.quit_key else 'Expert\n')+'ReadDelay=0\n')
    diagnostics='' if a.quit_key else 'COREDUMP '
    (boot/'s/startup-sequence').write_text('DF0:C/Assign C: DF0:C\nDF0:C/Assign LIBS: DF0:Libs\nDF0:C/Assign DEVS: DH0:devs\nStack 16384\nFailAt 999\nCD DH1:\n'+f'WHDLoad PrinceOfPersia2.slave {options}SPLASHDELAY=0 NOREQ {diagnostics}{timeout}>DH0:result\n'+'If WARN\nEcho failed >DH0:failed\nElse\nEcho passed >DH0:passed\nEndIf\nEcho returned >DH0:returned\n')
    (base/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    script=base/'launch.sh'
    script.write_text('''#!/bin/bash
set -euo pipefail
cd "$1"
. "$2/amiga/env.sh"
FSUAE_RUN=.
. "$FSUAE_COMMON"
trap 'fsuae_stop' EXIT
FSUAE="${FSUAE_WHDLOAD:-$FSUAE_MMU}"
AMIGA_SETUP="$3" DEBUG="$4" SOUND=0 WINDOW=none WARP=1 NTSC=0
fsuae_options
echo "$DEBUG_PORT" > debug-port
fsuae_launch --hard_drive_0="$PWD/boot" --hard_drive_0_priority=10 --hard_drive_1="$PWD/game" --floppy_drive_0="$WORKBENCH_ADF" --floppy_drive_volume=0 --floppy_drive_volume_empty=0 --state_dir="$PWD/state" --logs_dir="$PWD/logs" --joystick_port_0=nothing --joystick_port_1=nothing
wait "$FSUAE_PID"
''')
    try:
        with (base/'emulator.log').open('w') as log:
            emu=subprocess.Popen(['/bin/bash',str(script),str(base),str(ROOT),a.setup,'1' if debug else '0'],stdout=log,stderr=log)
            if a.gameplay:
                from whdload_gameplay import run
                run(base,a.seconds)
                return
            if a.store_reference:
                from whdload_store import run
                run(base,a.store_reference,a.seconds)
                return
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
