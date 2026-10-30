"""Bounded debugger fixture for the original CODE 20 indirect store."""
import hashlib
import os
from pathlib import Path
import shlex
import subprocess
import time
from extract_original_data import ROOT, pins, verify_output, DESTINATION
from check_indirect_store import check


def prepare(base):
    import _amigaxdev
    from amigaxdev.mac.resource_fork import read_resource_fork
    verify_output(DESTINATION)
    code = next(r.body for r in read_resource_fork(DESTINATION/'Prince of Persia 2.rsrc')
                if r.kind == b'CODE' and r.rid == 20)
    ram = (base/'before-fast.bin').read_bytes()
    candidates = []
    at = ram.find(code[:20])
    while at >= 0:
        if ram[at+0x46b4:at+0x46b8] == bytes.fromhex('aff94e71'):
            candidates.append(at)
        at = ram.find(code[:20], at+1)
    assert len(candidates) == 1, 'STORE / LOADED CODE 20'
    address = 0x200000+candidates[0]+0x46b8
    (base/'store-next.gdb').write_text(
        f'break *{address-4:#x}\ncondition 1 $a0==0 && $d0==0xffffffff\ncontinue\n'
        'dump binary memory store-vector.bin 0x20 0x24\n'
        'printf "STORE_BEFORE sr=%04lx\\n",$sr\ndisable 1\n'
        f'break *{address:#x}\ncontinue\n'
        'info registers\nx/wx 0x20\nx/wx $a5+0x2020\n'
        'dump binary memory after-fast.bin 0x200000 0xa00000\n'
        'dump binary memory after-chip.bin 0 0x200000\n')


def emulator(cache_memory=False, joystick=False):
    # Joystick runs need the current shared build's `monitor input`.
    if joystick:
        path = Path(os.environ['FSUAE_ARM'])
        name = 'fs-uae-arm'
    else:
        path = Path(os.environ.get('FSUAE_WHDLOAD') or os.environ['FSUAE_MMU'])
        name = 'fs-uae-cache-observer' if cache_memory else 'fs-uae-pop2-release'
    expected = pins(ROOT/'manifest/release-debug.sha256')[name]
    assert hashlib.sha256(path.read_bytes()).hexdigest() == expected, 'STORE / DEBUGGER EMULATOR PIN'
    return path


def run(base, original, seconds):
    emulator()
    for _ in range(100):
        if (base/'debug-port').exists():
            port = (base/'debug-port').read_text().strip()
            if subprocess.run(['lsof','-nP','-iTCP:'+port,'-sTCP:LISTEN'],capture_output=True).returncode == 0:
                break
        time.sleep(.2)
    else:
        raise AssertionError('STORE / DEBUGGER LISTENER')
    # Keep one connection: this stub closes its listener after detach.
    commands = ('set pagination off\nset confirm off\nset remotetimeout 300\n'
                f'target remote 127.0.0.1:{port}\nmonitor fields 1200\n'
                'dump binary memory before-fast.bin 0x200000 0xa00000\n'
                'dump binary memory before-chip.bin 0 0x200000\n'
                'shell python3 '+shlex.quote(str(Path(__file__).resolve()))+' --prepare\n'
                'source store-next.gdb\ndetach\nquit\n')
    (base/'store.gdb').write_text(commands)
    home = base/'home'; home.mkdir(exist_ok=True)
    env = dict(os.environ, HOME=str(home), XDG_CACHE_HOME=str(home))
    with (base/'gdb.log').open('w') as output:
        subprocess.run(['m68k-amiga-elf-gdb','--batch','-q','-x','store.gdb'],
                       cwd=base, env=env, stdout=output, stderr=output, check=True, timeout=seconds)
    check(original, base)


if __name__ == '__main__':
    import sys
    assert sys.argv[1:] == ['--prepare']
    prepare(Path.cwd())
