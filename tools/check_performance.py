"""Validate a bounded profiling window, not the full integration/timing gate."""
import argparse
import hashlib
import re
import subprocess
from pathlib import Path
from check_core_result import restored
from check_integration_result import crop, reference_state
from check_display_scenes import decode_planes
from extract_original_data import ROOT, pins


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


def route(log):
    rows = re.findall(r'^PROFILE ROUTE C2P frames=(\d+) passes=(\d+) rects=(\d+) pixels=(\d+) unbatched=(\d+)$',log,re.M)
    require(len(rows) == 1, 'FULL ROUTE C2P RECORD')
    frames, passes, rectangles, pixels, unbatched = map(int,rows[0])
    require(frames >= 5 and 0 < passes <= frames and rectangles >= passes and pixels > 0, 'FULL ROUTE FRAME-END PASSES')
    require(unbatched == 0, 'FULL ROUTE UNBATCHED CONVERSION')
    return frames, passes


def parent_identity(actual):
    expected = pins(ROOT/'manifest/performance-parent.sha256')
    require(set(expected) == {'parent-profile.elf','parent-profile.exe','parent-context.bin','parent-screen.bin','parent-death-context.bin','parent-death-screen.bin'}, 'PARENT PIN COVERAGE')
    require(actual == expected, 'PINNED PARENT BUILD AND STATE')


def check(run, reference, setup, compare=None, controls=False):
    reference_state(reference)
    log = (run/'gdb-out.log').read_text()
    fields = window(log)
    route_frames, route_passes = route(log)
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
    require(decode_planes((run/'profile-end-planes.bin').read_bytes()) == crop(screen), 'END WINDOW DIRTY PLANES')
    require(decode_planes((run/'death-planes.bin').read_bytes()) == crop((run/'death-screen.bin').read_bytes()), 'END ROUTE DIRTY PLANES')
    if compare:
        files = {'parent-profile.elf':compare/'PrinceOfPersia2.elf',
                 'parent-profile.exe':compare/'dh1/PrinceOfPersia2',
                 'parent-context.bin':compare/'profile-end-context.bin',
                 'parent-screen.bin':compare/'profile-end-screen.bin',
                 'parent-death-context.bin':compare/'death-context.bin',
                 'parent-death-screen.bin':compare/'death-screen.bin'}
        parent_identity({name:hashlib.sha256(path.read_bytes()).hexdigest() for name,path in files.items()})
        require(screen == (compare/'profile-end-screen.bin').read_bytes(), 'MATCHED END SCREEN')
        require(re.findall(r'^PROFILE C2P .*$',log,re.M) == re.findall(r'^PROFILE C2P .*$',(compare/'gdb-out.log').read_text(),re.M), 'MATCHED DIRTY WORK')
        require(state == (compare/'profile-end-context.bin').read_bytes(), 'MATCHED END CONTEXT')
        for name in ('death-context.bin','death-screen.bin'):
            require((run/name).read_bytes() == (compare/name).read_bytes(), 'MATCHED ROUTE END '+name)
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
        for old,new in [('PROFILE ROUTE C2P','MISSING ROUTE C2P'),('unbatched=0','unbatched=1')]:
            require(old in log, 'ACTIVE ROUTE CONTROL')
            try:
                route(log.replace(old,new))
            except RuntimeError:
                continue
            raise RuntimeError('PERFORMANCE / ACCEPTED ALTERED ROUTE')
        expected = pins(ROOT/'manifest/performance-parent.sha256')
        parent_identity(expected)
        for name in expected:
            bad = dict(expected);bad[name] = ('0' if bad[name][0] != '0' else '1')+bad[name][1:]
            try:
                parent_identity(bad)
            except RuntimeError:
                continue
            raise RuntimeError('PERFORMANCE / ACCEPTED ALTERED PARENT')
        print(f'PERFORMANCE CONTROLS PASS: {len(mutations)+2+len(expected)} altered window/route/parent records rejected')
    print(f'PERFORMANCE WINDOW PASS: five updates, {fields} PAL fields, {fields*4} ms/update; end context SHA256 {hashlib.sha256(state).hexdigest()}')
    print(f'PERFORMANCE ROUTE PASS: {route_passes} conversion passes over {route_frames} updates; none outside frame end')
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
