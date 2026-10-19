"""Real Installer 43 workflow with scripted requester answers (not UI coverage).

Adapted from Alone in the Dark. No helper/copy/rename/error operation is mocked.
"""
import argparse,hashlib,json,os,shutil,subprocess,sys,tempfile,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools'))
sys.path.insert(0,str(Path(__file__).parent))
from test_install import EXPECTED,SOURCE

def replace_form(text,start,replacement):
    a=text.index(start);depth=0;quoted=False;i=a
    while i<len(text):
        c=text[i]
        if quoted and c=='\\':i+=2;continue
        if c=='"':quoted=not quoted
        elif not quoted:
            if c=='(':depth+=1
            elif c==')':
                depth-=1
                if depth==0:return text[:a]+replacement+text[i+1:]
        i+=1
    raise ValueError('Unbalanced Installer form')

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--case',choices=['fresh','reuse','replace','incomplete','bad-options','bad-archive'],default='reuse')
    p.add_argument('--seconds',type=int,default=900,help='positive wall-clock limit for Installer to return')
    p.add_argument('--window',action='store_true',help='show the emulator for manual observation; requester answers remain scripted')
    p.add_argument('--interactive-error',action='store_true',
                   help='show the real bad-archive requester for manual dismissal')
    a=p.parse_args()
    if a.seconds<=0:p.error('--seconds must be positive')
    if a.interactive_error and a.case!='bad-archive':p.error('--interactive-error requires --case bad-archive')
    from scaffold_inputs import verify
    for group in ('run','build','installer-run'):verify(group)
    base=Path(tempfile.mkdtemp(prefix='installer43-',dir=ROOT/'tmp'))
    print(base,flush=True)
    boot=base/'boot';out=base/'out';source=base/'source';dest=out/'Prince of Persia 2'
    for d in [boot/'s',base/'state',out,source]:d.mkdir(parents=True,exist_ok=True)
    subprocess.run(['lha','xq',str(ROOT/'dist/PrinceOfPersia2-0.90.lha')],cwd=base,check=True)
    package=base/'Prince of Persia 2 Install'
    for f in package.iterdir():
        if f.is_dir():shutil.copytree(f,boot/f.name)
        else:shutil.copyfile(f,boot/f.name)
    shutil.copyfile(SOURCE,source/'original.sit')
    if a.case=='bad-archive':(source/'original.sit').write_bytes(b'damaged archive')
    if a.case!='fresh':
        subprocess.run([str(ROOT/'build/install-data/Pop2InstallData'),str(SOURCE),str(dest)],check=True)
        (dest/'keep-marker').write_text('old installation')
        (dest/'Data/Fixture Save').write_bytes(b'preserve unrelated saved-game fixture')
        for n in ['PrinceOfPersia2','PrinceOfPersia2.slave','ReadMe']:(dest/n).write_bytes(b'old release')
        if a.case in ['replace','bad-archive']:(dest/'Prince of Persia 2.rsrc').write_bytes(b'old damaged original')
        if a.case=='incomplete':(dest/'Data/Kid.rsrc.rsrc').unlink()
        if a.case=='bad-options':(dest/'Data/Prince2.opt').write_bytes(b'damaged preferences')
    (out/'other').mkdir();(out/'other/keep').write_bytes(b'unrelated')
    before={str(f.relative_to(dest)):f.read_bytes() for f in dest.rglob('*') if f.is_file()} if dest.exists() else {}
    (base/'before.json').write_text(json.dumps({n:hashlib.sha256(b).hexdigest() for n,b in before.items()},indent=2)+'\n')
    build=ROOT/'build/install-data'
    subprocess.run(['m68k-amiga-elf-gcc','-O2','-m68020','-nostdlib','-Wno-volatile-register-var','-Wl,--emit-relocs,-Ttext=0,-e,_start',str(ROOT/'tools/install-data/start.s'),str(ROOT/'tools/install-data/test_icon.c'),'-o',str(build/'test-icon.elf')],check=True)
    subprocess.run([os.environ['ELF2HUNK'],str(build/'test-icon.elf'),str(build/'test-icon.exe'),'-s'],check=True)
    for src,n in [(Path(os.environ['INSTALLER43']),'Installer'),(Path(os.environ['WHDLOAD_BINARY']),'WHDLoad'),(build/'test-icon.exe','IconTest')]:shutil.copyfile(src,boot/n)
    shutil.copyfile(boot/'PrinceOfPersia2.inf',boot/'GameTemplate.info');(boot/'Package').mkdir();shutil.copyfile(base/'Prince of Persia 2 Install.info',boot/'Package.info')
    script=(boot/'Install').read_text()
    script=replace_form(script,'(welcome)','(if 0 (welcome))')
    script=replace_form(script,'(set #parent','(set #parent "DH2:out")')
    script=replace_form(script,'(set #archive','((textfile (dest "DH2:asked-archive") (append "asked")) (set #archive "DH1:original.sit"))')
    script=replace_form(script,'(set #reuse\n       (askbool','((textfile (dest "DH2:asked-reuse") (append "asked")) (set #reuse 1))')
    script=replace_form(script,'(exit)','(textfile (dest "DH2:completed") (append "complete"))\n(exit (quiet))')
    (boot/'Install').write_text(script)
    (boot/'s/startup-sequence').write_text('CD DH0:\nStack 16384\nIconTest\nDF0:C/Assign C: DF0:C\nDF0:C/Assign LIBS: DF0:Libs\nPath DH0: ADD\nC:LoadWB\nInstaller SCRIPT DH0:Install APPNAME "Prince of Persia 2" MINUSER NOVICE DEFUSER NOVICE LOGFILE DH2:installer.log NOPRETEND >DH2:console.log\nIconTest\nEcho returned >DH2:returned\n')
    launch=base/'launch.sh';launch.write_text('''#!/bin/bash
set -euo pipefail
cd "$1"
. "$2/amiga/env.sh"
FSUAE_RUN=.
. "$FSUAE_COMMON"
trap 'fsuae_stop' EXIT
FSUAE="$FSUAE_ARM"
AMIGA_SETUP=a1200-030 DEBUG=0 SOUND=0 WINDOW="${INSTALLER_WINDOW:-none}" WARP=1 NTSC=0
EXTRA_ARGS='--uae_cpu_speed=max --uae_cpu_cycle_exact=false --uae_cpu_memory_cycle_exact=false'
fsuae_options
fsuae_launch --hard_drive_0="$PWD/boot" --hard_drive_0_priority=10 --hard_drive_1="$PWD/source" --hard_drive_2="$PWD" --floppy_drive_0="$WORKBENCH_ADF" --floppy_drive_volume=0 --floppy_drive_volume_empty=0 --state_dir="$PWD/state"
wait "$FSUAE_PID"
''')
    try:
        with (base/'emulator.log').open('w') as log:
            env=dict(os.environ,INSTALLER_WINDOW='front' if a.interactive_error or a.window else 'none')
            emu=subprocess.Popen(['/bin/bash',str(launch),str(base),str(ROOT)],stdout=log,stderr=log,env=env)
            deadline=time.monotonic()+a.seconds
            while time.monotonic()<deadline and not (base/'returned').exists():
                if emu.poll() is not None:raise RuntimeError('Installer emulator exited')
                time.sleep(.5)
        assert (base/'returned').exists(),'Installer did not return'
        assert (base/'icon-ok').exists(),'Template icon failed native validation'
        assert (out/'other/keep').read_bytes()==b'unrelated'
        if a.case=='bad-archive':
            assert not (base/'completed').exists()
            assert before=={str(f.relative_to(dest)):f.read_bytes() for f in dest.rglob('*') if f.is_file() and not f.name.endswith('.uaem')}
        else:
            assert (base/'completed').exists(),'Installer did not reach completion'
            for name,digest in EXPECTED.items():assert hashlib.sha256((dest/name).read_bytes()).hexdigest()==digest,name
            assert (base/'installed-icon-ok').exists(),'Installed icon failed native validation'
            for n in ['PrinceOfPersia2','PrinceOfPersia2.slave','ReadMe','ReadMe.info']:assert (dest/n).read_bytes()==(boot/n).read_bytes(),n
            assert (base/'asked-archive').exists()==(a.case!='reuse')
            assert (base/'asked-reuse').exists()==(a.case=='reuse')
            if before:
                backup=out/'.replaced/PrinceOfPersia2-0'
                for n,b in before.items():assert (backup/n).read_bytes()==b,n
                assert (dest/'Data/Fixture Save').read_bytes()==b'preserve unrelated saved-game fixture'
            assert not list(out.glob('.pop2-stage-*'))
        print('INSTALLER 43 PASS:',a.case,flush=True)
        (base/'result.json').write_text(json.dumps({'case':a.case,'interactive_error':a.interactive_error,'seconds':a.seconds,
            'returned':True,'preservation_checked':bool(before),'status':'pass'},indent=2)+'\n')
    finally:
        subprocess.run(['/bin/bash','-c','. "$1/amiga/env.sh"; FSUAE_RUN="$2"; . "$FSUAE_COMMON"; fsuae_stop_previous','stop',str(ROOT),str(base)],check=True)
        if 'emu' in locals():emu.wait(timeout=10)
if __name__=='__main__':main()
