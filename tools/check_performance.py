"""Validate a bounded profiling window, not the full integration/timing gate."""
import argparse
import hashlib
import re
import subprocess
from pathlib import Path
from check_core_result import restored
from check_integration_result import crop, reference_state
from check_display_scenes import decode_planes


def require(ok, reason):
    if not ok:
        raise RuntimeError('PERFORMANCE / ' + reason)


def window(log):
    start = re.findall(r'^PROFILE START fields=(\d+) ticks=(\d+) rows=(\d+) right_reads=(\d+)$', log, re.M)
    end = re.findall(r'^PROFILE COMPLETE fields=(\d+) ticks=(\d+) frames=(\d+)$', log, re.M)
    require(len(start) == len(end) == 1, 'UNIQUE WINDOW')
    before, tick_before, rows, right = map(int, start[0])
    after, tick_after, frames = map(int, end[0])
    require(rows == 1 and frames == 6, 'FIVE COMPLETE UPDATES')
    require(right == 0 and log.count('PROFILE INPUT right_reads=0\n') == 1, 'NO WINDOW INPUT')
    require(after > before and tick_after > tick_before, 'CLOCK PROGRESS')
    require(log.count('CLOCK FAULT 0\n') == 1, 'MONOTONIC CLOCK')
    require(log.count('CORE REASON INTEGRATION / ROUTE COMPLETE\n') == 1, 'ROUTE COMPLETION')
    require(log.count('CORE EXCEPTION vector=0 pc=00000000\n') == 1, 'EXCEPTION')
    require(log.count('INTEGRATION AUDIT code=0 input=1\n') == 1, 'EXPLICIT AUDIT MODE')
    require(log.count('INTEGRATION RESTORED\n') == 1, 'RESTORATION')
    require(re.findall(r'^INTEGRATION STAGE (\d+) fields=\d+ right=\d+ kind=\d+$', log, re.M) == ['1','2','3','4'], 'ORDERED ROUTE')
    c2p = re.findall(r'^PROFILE C2P passes=(\d+) rects=(\d+) pixels=(\d+) unbatched=(\d+)$', log, re.M)
    require(len(c2p) == 1, 'C2P WINDOW')
    passes, rectangles, pixels, unbatched = map(int,c2p[0])
    require(passes == 5 and rectangles >= passes and 0 < pixels <= 5*512*384, 'ONE DIRTY CONVERSION PASS PER UPDATE')
    require(unbatched == 0, 'C2P ONLY AT FRAME END')
    return after-before


def check(run, reference, setup, compare=None, controls=False):
    reference_state(reference)
    log = (run/'gdb-out.log').read_text()
    fields = window(log)
    # Profiling deliberately omits exit CODE/heap audits; require that explicit
    # mode, while checking actual machine settings and restored hardware.
    restored(run, log, setup, heap_checked=False)
    for name in ('exe','elf'):
        path = run/('dh1/PrinceOfPersia2' if name == 'exe' else 'PrinceOfPersia2.elf')
        expected = [line.split()[0] for line in (run/'build.sha256').read_text().splitlines()
                    if line.split()[-1].endswith('/'+path.name)]
        require(len(expected) == 1 and hashlib.sha256(path.read_bytes()).hexdigest() == expected[0], 'FROZEN '+name)
    symbols = subprocess.check_output(['m68k-amiga-elf-objdump','-t',str(run/'PrinceOfPersia2.elf')], text=True)
    names = {line.split()[-1] for line in symbols.splitlines() if len(line.split()) >= 6}
    require({'g_timingCount','g_integrationStage'} <= names, 'MEASUREMENT PROBES')
    require(not names.intersection({'g_coreTrace','g_coreCount','g_fileVerifiedFiles','g_fileVerifiedBytes'}), 'NO VERIFICATION COUNTERS')
    require(not any(re.match(r'_Z[0-9]+sha_(init|update|final)', name) for name in names), 'NO RUNTIME SHA256')
    expected = crop((reference/'first-room.indices').read_bytes())
    require(crop((run/'first-room-screen.bin').read_bytes()) == expected, 'INITIAL PIXELS')
    require(decode_planes((run/'first-room-planes.bin').read_bytes()) == expected, 'INITIAL PLANES')
    colors = (run/'first-room-colors.bin').read_bytes()
    require(len(colors) == 2056 and colors[4:] == (reference/'first-room.clut').read_bytes()[4:], 'INITIAL COLORS')
    for filename, size, span in [('first-room-a5.bin',22266,slice(-0x18fc,-0x18f8)),
                                ('first-room-context.bin',0x2200,slice(0x2180,0x21ba))]:
        data = (run/filename).read_bytes()
        require(len(data) == size and data[span] == (reference/filename).read_bytes()[span], 'INITIAL STATE '+filename)
    state = (run/'profile-end-context.bin').read_bytes()
    require(len(state) == 0x2200, 'END CONTEXT EXTENT')
    screen = (run/'profile-end-screen.bin').read_bytes()
    require(len(screen) == 640*480, 'END SCREEN EXTENT')
    if compare:
        require(screen == (compare/'profile-end-screen.bin').read_bytes(), 'MATCHED END SCREEN')
        require(re.findall(r'^PROFILE C2P .*$',log,re.M) == re.findall(r'^PROFILE C2P .*$',(compare/'gdb-out.log').read_text(),re.M), 'MATCHED DIRTY WORK')
        require(state == (compare/'profile-end-context.bin').read_bytes(), 'MATCHED END CONTEXT')
        require(window((compare/'gdb-out.log').read_text()) > 0, 'COMPARISON WINDOW')
    if controls:
        mutations = [('rows=1 right_reads=0','rows=2 right_reads=0'),
                     ('PROFILE INPUT right_reads=0','PROFILE INPUT right_reads=1'),
                     ('CLOCK FAULT 0','CLOCK FAULT 1'),
                     ('INTEGRATION / ROUTE COMPLETE','INTEGRATION / TIMEOUT'),
                     ('INTEGRATION AUDIT code=0 input=1','INTEGRATION AUDIT code=0 input=0'),
                     ('INTEGRATION RESTORED','NOT RESTORED'),
                     ('PROFILE C2P passes=5','PROFILE C2P passes=6'),
                     ('unbatched=0','unbatched=1')]
        for old, new in mutations:
            require(old in log, 'ACTIVE CONTROL')
            try:
                window(log.replace(old,new))
            except RuntimeError:
                continue
            raise RuntimeError('PERFORMANCE / ACCEPTED ALTERED WINDOW')
        window(log)
        print(f'PERFORMANCE CONTROLS PASS: {len(mutations)} altered windows rejected')
    print(f'PERFORMANCE WINDOW PASS: five updates, {fields} PAL fields, {fields*4} ms/update; end context SHA256 {hashlib.sha256(state).hexdigest()}')
    print('Instrumented measurement only; title pairing, exit CODE/heap audits and full timing acceptance are not established.')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run',type=Path)
    parser.add_argument('reference',type=Path)
    parser.add_argument('--setup',required=True,choices=['a1200-fast','a1200-030','a1200-040'])
    parser.add_argument('--compare',type=Path)
    parser.add_argument('--controls',action='store_true')
    args = parser.parse_args()
    check(args.run,args.reference,args.setup,args.compare,args.controls)
