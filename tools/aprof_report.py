#!/usr/bin/env python3
"""Summarize the opening-sequence cycle profile written by amiga/aprof.sh.

  aprof_report.py check-run RUN                    completion gate used by amiga/run.sh
  aprof_report.py opening NAME [--compare BASE] [--original REF]  phases, cadence, repeat check
  aprof_report.py report NAME [--phase N]          buckets, chip bus, flat, inclusive, traps
  aprof_report.py annotate NAME FUNCTION [--phase N]  hottest instructions of a native function
  aprof_report.py calls NAME LABEL [--parent LABEL] [--phase N]  inclusive cost per call
  aprof_report.py subtree NAME LABEL... [--phase N]  original/port time below call-tree labels
  aprof_report.py work NAME [--phase N] [--exclude LABEL]  cost outside the timer wait loop

Inputs are tmp/aprof/NAME/. Without --phase, reports sum every phase from the first
room to the death song. Times are emulated: the profiler charges FS-UAE cycle units,
wait states included, to every instruction's PC, so warp does not matter. Original
functions are CODEn+$offset (offsets include the CODE header, as in symbols.csv) from
LINK A6 prologues and observed call targets; MacsBug names are added where the
original's name record follows the function. Adapted from Alone in the Dark's report.
"""
import argparse, bisect, collections, csv, hashlib, json, re, struct, subprocess, sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'tmp/aprof'
APP = ROOT / 'tmp/runtime-data/Prince of Persia 2.rsrc'
OBJDUMP = 'm68k-amiga-elf-objdump'
LINE_A = 10
ROW_LONGS = 20
STALL_FIELDS = 25  # 0.5 s of PAL fields, as in Alone in the Dark

# Exclusive self-time buckets for native code, by function-name fragment (first match).
BUCKETS = [
    ('Presentation: C2P', ('c2p1x1_8_c5_', 'KalmsC2P', 'CachedC2PRect', 'c2pRect')),
    ('Presentation: other', ('NativeDisplay::', 'PrinceScreen::', 'Planar8', 'DirtyBands::', 'AmigaHardware::blitter', 'AmigaHardware::waitBlit',
                             'paceFrame', 'publish')),
    ('QuickDraw: pure rectangles', ('pop2SectRect', 'pop2UnionRect', 'pop2EmptyRect', 'RectBounds')),
    ('QuickDraw: CopyBits', ('CopyBits8', 'RegionRows', 'RegionCopy', 'copyBits', 'CopyBits', 'memmove', 'memcpy', 'MaskCopy')),
    ('QuickDraw: fills, lines, regions, text', ('FillRect8', 'Line8', 'Region', 'Polygon', 'BitmapText8', 'BitmapFont', 'PenRect')),
    ('QuickDraw: GWorld, port and palette state', ('GWorld8', 'Palette8', 'ColorMap8', 'AnimatePalette8', 'gWorld', 'GWorld')),
    ('Memory Manager', ('MacHeap', 'HandleCopy', 'blockMove', 'BlockMove')),
    ('Files and resources', ('MacFiles', 'FileReadCache', 'ResourceDirectory', 'NativeFiles', 'NativeResources', 'MacVolume')),
    ('Audio', ('Halestorm', 'Paula', 'NativeAudio', 'PcmDma', 'audio', 'Audio')),
    ('Input', ('NativeInput', 'InputState', 'getKeys', 'GetKeys')),
    ('Trap entry and dispatch', ('pop2Quick', 'pop2_line_a', 'pop2_cached_services', 'dispatch', 'Dispatch', 'trap', 'Trap',
                                 'CodeRangeCache', 'pop2_timer_callback', 'service', 'Service')),
    ('Timing, VBL and interrupts', ('VBL', 'vbl', 'Vbl', 'VideoTicks', 'wallTicks', 'callbackTicks', 'Time', 'interrupt', 'Interrupt')),
]


def elf_section(path, wanted):
    """Contents of one section of a big-endian ELF32 file."""
    data = Path(path).read_bytes()
    shoff, = struct.unpack_from('>I', data, 0x20)
    shentsize, shnum, shstrndx = struct.unpack_from('>HHH', data, 0x2E)
    header = lambda i: struct.unpack_from('>10I', data, shoff + i * shentsize)
    names = header(shstrndx)
    for i in range(shnum):
        h = header(i)
        name = data[names[4] + h[0]:data.index(b'\0', names[4] + h[0])].decode()
        if name == wanted:
            return data[h[4]:h[4] + h[5]]
    raise SystemExit(f'APROF / no ELF section {wanted}')


def require(ok, why):
    if not ok:
        raise SystemExit('APROF / ' + why)


# ---- run completion -------------------------------------------------------

def machine(run):
    config = dict(l.split('=', 1) for l in (run / 'logs/debug.uae').read_text().splitlines() if '=' in l)
    keys = ('cpu_model', 'mmu_model', 'cpu_frequency', 'cpu_cycle_exact', 'cpu_memory_cycle_exact',
            'cpu_data_cache', 'chipset', 'chipmem_size', 'fastmem_size', 'ntsc', 'cachesize')
    return {k: config.get(k) for k in keys}


def check_run(run):
    """Positive completion of the opening route; a timeout or missing stage fails."""
    log = (run / 'gdb-out.log').read_text(errors='replace')
    for text, why in (('CORE REASON INTEGRATION / ROUTE COMPLETE\n', 'ROUTE COMPLETION'),
                      ('CORE EXCEPTION vector=0 pc=00000000\n', 'EXCEPTION'),
                      ('INTEGRATION AUDIT code=0 input=1\n', 'PROFILE AUDIT MODE: INPUT RESTORED, NO EXIT CODE AUDIT'),
                      ('INTEGRATION INPUT right_reads=0\n', 'NO-INPUT ROUTE MUST READ NO RIGHT'),
                      ('CLOCK FAULT 0\n', 'CLOCK FAULT'),
                      ('INTEGRATION RESTORED\n', 'RESTORATION MILESTONE')):
        require(text in log, why)
    m = re.search(r'^CORE RESTORE stage=3 error=0 vector=1 heap=0 zone=3145728 dma=(\d+)/(\d+) irq=(\d+)/(\d+) view=1 vbi=1$', log, re.M)
    require(m and m[1] == m[2] and m[3] == m[4], 'RESTORED HARDWARE')
    stages = re.findall(r'^INTEGRATION STAGE (\d+) fields=\d+ right=\d+ kind=(\d+)$', log, re.M)
    require([int(s) for s, _ in stages] == [1, 2, 3, 4] and stages[-1][1] == '3', 'ORDERED STAGES AND KID DEATH SONG')
    rec = re.search(r'^OPENING RECORDED rows=(\d+) phases=(\d+)$', log, re.M)
    require(rec and 0 < int(rec[1]) < 1024, 'FRAME RECORD EXTENT')
    phases = re.findall(r'^OPENING PHASE (\d+) ', log, re.M)
    require([int(p) for p in phases] == list(range(int(rec[2]))), 'PHASE BOUNDARIES')
    require((run / 'opening-frames.bin').stat().st_size == int(rec[1]) * ROW_LONGS * 4, 'FRAME RECORD DUMP')
    profiles = sorted(run.glob('opening-*.prof'))
    scoped = re.search(r'^ACTIVE SCOPE COMPLETE waits=(\d+) parts=(\d+)$', log, re.M)
    if scoped:
        entered = re.findall(r'^ACTIVE WAIT ENTER (\d+) phase=(\d+) sp=[0-9a-f]+ return=[0-9a-f]+$', log, re.M)
        exited = re.findall(r'^ACTIVE WAIT EXIT (\d+) phase=(\d+)$', log, re.M)
        waits = sorted(run.glob('wait-*.prof'))
        require(sorted(int(p.stem.split('-')[1]) for p in waits) == list(range(1, int(scoped[1]) + 1)), 'WAIT ACCOUNTING PARTS')
        require(entered == exited and len(entered) == int(scoped[1]) > 0, 'BALANCED WAIT SCOPE')
        require([int(n) for n, _ in entered] == list(range(1, len(entered) + 1)), 'ORDERED WAIT SCOPE')
        require(len(profiles) == int(scoped[2]) == len(entered) + int(rec[2]) + 1, 'ACTIVE PROFILE COUNT')
        parts = [re.fullmatch(r'opening-(\d+)-(\d+)\.prof', p.name) for p in profiles]
        require(all(parts), 'ACTIVE PROFILE NAMES')
        require(sorted(int(p[2]) for p in parts) == list(range(len(parts))), 'ACTIVE PROFILE PARTS')
        require(set(int(p[1]) for p in parts) == set(range(int(rec[2]) + 1)), 'ACTIVE PROFILE PHASES')
        expected = []
        current_phase = 0
        pending = None
        for line in log.splitlines():
            entry = re.match(r'ACTIVE WAIT ENTER (\d+) phase=(\d+) ', line)
            leave = re.match(r'ACTIVE WAIT EXIT (\d+) phase=(\d+)$', line)
            boundary = re.match(r'OPENING PHASE (\d+) ', line)
            finish = line.startswith('ACTIVE SCOPE COMPLETE ')
            if entry:
                require(pending is None and int(entry[2]) == current_phase, 'WAIT ENTRY ORDER')
                pending = int(entry[1])
            elif leave:
                require(pending == int(leave[1]) and int(leave[2]) == current_phase, 'WAIT EXIT ORDER')
                pending = None
            elif boundary or finish:
                require(pending is None, 'BOUNDARY INSIDE WAIT')
                if boundary:
                    require(int(boundary[1]) == current_phase, 'SCOPE PHASE ORDER')
            if entry or boundary or finish:
                expected.append(f'opening-{current_phase}-{len(expected)}.prof')
                if boundary:
                    current_phase += 1
        require(set(expected) == {p.name for p in profiles}, 'PROFILE PART PHASE OWNERSHIP')
    else:
        require('ACTIVE WAIT' not in log, 'MISSING ACTIVE SCOPE COMPLETION')
        require(len(profiles) in (0, int(rec[2]) + 1), 'PHASE PROFILE COUNT')
    print(f'OPENING RUN PASS: {rec[1]} frames, {int(rec[2]) + 1} phases, {len(profiles)} profiles; {machine(run)}')


# ---- frame records --------------------------------------------------------

def frames(directory):
    data = (directory / 'opening-frames.bin').read_bytes()
    rows = []
    for i in range(len(data) // (ROW_LONGS * 4)):
        v = struct.unpack_from('>20I', data, i * ROW_LONGS * 4)
        kid = data[i * ROW_LONGS * 4 + 16:i * ROW_LONGS * 4 + 16 + 62]
        word = lambda off: struct.unpack_from('>h', kid, off)[0]
        rows.append(dict(counter=v[0], ticks=v[1], fields=v[2], phase=v[3],
                         x=word(4), y=word(6), room=word(0x16), health=word(0x1e), kid=kid.hex()))
    return rows


def start_fields(log):
    m = re.search(r'^PROFILE START fields=(\d+) ticks=(\d+)', log, re.M)
    require(m, 'NO PROFILE START')
    return int(m[1]), int(m[2])


def stop_fields(log):
    m = re.search(r'^PROFILE STOP fields=(\d+) ticks=(\d+) frames=(\d+) phases=(\d+)$', log, re.M)
    require(m, 'NO PROFILE STOP')
    return int(m[1]), int(m[2]), int(m[3]), int(m[4])


def summarize(name):
    d = OUT / name
    log = (d / 'gdb-out.log').read_text(errors='replace')
    rows = frames(d)
    begin, _ = start_fields(log)
    end, _, count, phases = stop_fields(log)
    require(count == len(rows), 'FRAME COUNT AT STOP')
    previous = begin
    for r in rows:
        r['frame_fields'] = r['fields'] - previous
        previous = r['fields']
    out = {'name': name, 'config': (d / 'config.txt').read_text().strip(), 'frames': len(rows),
           'fields': end - begin, 'tail_fields': end - previous, 'phases': []}
    for p in range(phases + 1):
        sel = [r for r in rows if r['phase'] == p]
        if p == phases:
            tail = end - previous
        else:
            tail = 0
        fields = sum(r['frame_fields'] for r in sel) + tail
        stalls = [r for r in sel if r['frame_fields'] >= STALL_FIELDS]
        steady = [r['frame_fields'] for r in sel if r['frame_fields'] < STALL_FIELDS]
        out['phases'].append(dict(phase=p, frames=len(sel), fields=fields,
                                  health=sel[0]['health'] if sel else None,
                                  first_counter=sel[0]['counter'] if sel else None,
                                  steady_fields_per_frame=round(sum(steady) / len(steady), 3) if steady else None,
                                  max_frame_fields=max((r['frame_fields'] for r in sel), default=0),
                                  stalls=[(r['counter'], r['frame_fields']) for r in stalls]))
    hist = collections.Counter(r['frame_fields'] for r in rows)
    out['frame_field_histogram'] = dict(sorted(hist.items()))
    # Cadence is judged at the original SetTimer calls (CODE 17+313E), against
    # the delay each frame requested; the original asks for 6 ticks in combat.
    data = (d / 'timing-set.bin').read_bytes()
    sets = [struct.unpack_from('>5I', data, i) for i in range(0, len(data), 20)]
    out['requests'] = [s[3] for s in sets]
    gaps = [b[1] - a[1] for a, b in zip(sets, sets[1:])]
    out['late_intervals'] = [(i, g, sets[i][3]) for i, g in enumerate(gaps) if g > sets[i][3]]
    out['intervals'] = len(gaps)
    return out, rows


def original_requests(reference):
    """Timer-1 delays the original requested between first room and death."""
    log = (reference / 'session.log').read_text(errors='replace')
    require('AUDIO_PHASE gameplay' in log and 'AUDIO_ROUTE_COMPLETE' in log, 'ORIGINAL NO-INPUT ROUTE INCOMPLETE')
    play = log[log.index('AUDIO_PHASE gameplay'):log.index('AUDIO_PHASE death')]
    sets = [(int(t, 16), int(d, 16)) for t, timer, d in
            re.findall(r'TIMING_SET ticks=([0-9A-F]+) timer=([0-9A-F]+) delay=([0-9A-F]+)', play) if timer == '0001']
    gaps = [b[0] - a[0] for a, b in zip(sets, sets[1:])]
    require(all(g == sets[i][1] for i, g in enumerate(gaps)), 'ORIGINAL MISSED ITS OWN REQUEST')
    return [d for _, d in sets]


def opening(name, compare, original):
    out, rows = summarize(name)
    d = OUT / name
    with (d / 'frames.csv').open('w', newline='') as f:
        w = csv.writer(f)
        w.writerow(['index', 'counter', 'ticks', 'fields', 'frame_fields', 'phase', 'room', 'x', 'y', 'health', 'kid'])
        for i, r in enumerate(rows):
            w.writerow([i, r['counter'], r['ticks'], r['fields'], r['frame_fields'], r['phase'], r['room'], r['x'], r['y'], r['health'], r['kid']])
    (d / 'summary.json').write_text(json.dumps(out, indent=1) + '\n')
    print(f"== {name} ({out['config']}): {out['frames']} frames, {out['fields']} PAL fields "
          f"({out['fields'] / out['frames']:.2f} fields/frame); requested SetTimer intervals late "
          f"{len(out['late_intervals'])}/{out['intervals']}")
    runs = [(k, len(list(g))) for k, g in __import__('itertools').groupby(out['requests'])]
    print(f'requests (ticks x count): {runs}')
    if original:
        expected = original_requests(Path(original))
        # The original observer prints its gameplay marker after polling for the
        # first-room capture, one frame after the native stage-3 checkpoint.
        lead = len(out['requests']) - len(expected)
        require(lead in (0, 1) and out['requests'][lead:] == expected, 'REQUESTED DELAYS DIFFER FROM THE ORIGINAL')
        print(f'ORIGINAL REQUESTS MATCH: {len(expected)} timer-1 delays from {original} (native window starts {lead} earlier)')
    print('phase  health  frames  fields  fields/frame  steady  max  stalls(counter:fields)')
    for p in out['phases']:
        print(f"{p['phase']:5d}  {p['health']!s:>6}  {p['frames']:6d}  {p['fields']:6d}  "
              f"{p['fields'] / max(1, p['frames']):12.2f}  {p['steady_fields_per_frame']!s:>6}  {p['max_frame_fields']:3d}  "
              + ' '.join(f'{c}:{n}' for c, n in p['stalls']))
    print('fields per frame histogram:', out['frame_field_histogram'])
    if compare:
        base, base_rows = summarize(compare)
        require(len(base['phases']) == len(out['phases']), 'REPEAT PHASE COUNT')
        for a, b in zip(base['phases'], out['phases']):
            require(a['health'] == b['health'] and a['frames'] == b['frames'], f"REPEAT PHASE {a['phase']} STATE")
            # Field quantization: allow one PAL field in short phases.
            require(abs(a['fields'] - b['fields']) <= max(1, 0.02 * a['fields']), f"REPEAT PHASE {a['phase']} FIELDS")
            require([c for c, _ in a['stalls']] == [c for c, _ in b['stalls']], f"REPEAT PHASE {a['phase']} STALLS")
        same = [a['kid'] == b['kid'] for a, b in zip(base_rows, rows)]
        print(f'REPEAT PASS against {compare}: same phase states and stalls, fields within max(1, 2%); '
              f'{sum(same)}/{len(same)} identical Kid records')


# ---- cycle profiles -------------------------------------------------------

from amigaxdev import aprof as shared_aprof
# Verify the canonical implementation before parsing any capture.
_helper_pin=(ROOT/'manifest/profile-helpers.sha256').read_text().split()[0]
require(hashlib.sha256(Path(shared_aprof.__file__).read_bytes()).hexdigest()==_helper_pin,
        'SHARED PROFILE READER PIN')
call_tree = shared_aprof.call_tree

def parse_profile(text):
    # Retain this CLI's established failure contract for gate callers.
    try:
        return shared_aprof.parse_profile(text)
    except ValueError as error:
        raise SystemExit(str(error)) from error


def trap_names():
    names = {}
    path = ROOT / 'tmp/trap_names.lua'
    if path.exists():
        for m in re.finditer(r'\[0x([0-9A-F]{4})\] = "(\w+)"', path.read_text()):
            names[int(m.group(1), 16)] = m.group(2)
    return names


def trap_name(word, traps):
    if word in traps:
        return traps[word]
    masked = word & ~0x0400 if word & 0x0800 else word & 0xF8FF
    return traps.get(masked, 'A%03X' % (word & 0xFFF))


def bucket(region, function):
    if region == 'ORIG':
        return 'Original Mac game code'
    if region != 'NATIVE':
        return 'Kickstart and other'
    for name, keys in BUCKETS:
        if any(k in function for k in keys):
            return name
    return 'Other native'


def short(fn, n=80):
    fn = fn.replace('(anonymous namespace)::', 'anon::')
    # Template values can contain casts: strip the argument list only after
    # the template closes, retaining distinct indexed dispatch entries.
    start = fn.find('(', fn.rfind('>') + 1)
    if start >= 0:
        fn = fn[:start]
    return fn if len(fn) <= n else fn[:n - 3] + '...'


class Context:
    """Run-wide symbols, segments and jump table shared by every phase profile."""

    def __init__(self, name):
        self.dir = OUT / name
        require(self.dir.is_dir(), f'missing {self.dir}')
        self.log = (self.dir / 'gdb-out.log').read_text(errors='replace')
        self.segs = sorted((int(b, 16), int(b, 16) + int(s, 16), int(i))
                           for i, b, s in re.findall(r'^PROFILE SEG (\d+) ([0-9a-f]+) ([0-9a-f]+)$', self.log, re.M))
        stop = sorted((int(b, 16), int(b, 16) + int(s, 16), int(i))
                      for i, b, s in re.findall(r'^PROFILE ENDSEG (\d+) ([0-9a-f]+) ([0-9a-f]+)$', self.log, re.M))
        require(self.segs and stop == self.segs, 'CODE SEGMENTS MOVED DURING THE INTERVAL')
        self.seg_begins = [s[0] for s in self.segs]
        self.a5 = int(re.search(r'^PROFILE A5 ([0-9a-f]+)$', self.log, re.M)[1], 16)
        self.sections = {}
        for m in re.finditer(r'^\s*\[\d+\]\s+0x([0-9a-f]+)->0x([0-9a-f]+) at 0x[0-9a-f]+: (\S+) ', self.log, re.M):
            if int(m.group(1), 16):
                self.sections[m.group(3)] = (int(m.group(1), 16), int(m.group(2), 16))
        config = dict(l.split('=', 1) for l in (self.dir / 'config.txt').read_text().split())
        self.config = config
        self.hz = 50000000 if config['setup'] == 'a1200-030' else 14187580
        self.jt = {}
        data = (self.dir / 'jt.bin').read_bytes()
        for n in range(len(data) // 8):
            _, op, addr = struct.unpack('>HHI', data[8 * n:8 * n + 8])
            if op == 0x4EF9:
                self.jt[self.a5 + 34 + 8 * n] = (n, addr)
        self.traps = trap_names()
        self._symbols(self.dir / 'PrinceOfPersia2.elf')
        self._original_names()

    def _symbols(self, elf):
        heads = subprocess.run([OBJDUMP, '-h', str(elf)], capture_output=True, text=True, check=True).stdout
        self.link = {f[1]: int(f[3], 16) for f in (l.split() for l in heads.splitlines()) if len(f) >= 7 and f[0].isdigit()}
        table = subprocess.run([OBJDUMP, '-t', '-C', str(elf)], capture_output=True, text=True, check=True).stdout
        symbols = []
        for line in table.splitlines():
            m = re.match(r'([0-9a-f]{8}) (.{7}) (\S+)\s+[0-9a-f]{8} (.+)$', line)
            if not m or m.group(3) not in self.sections or not m.group(3).startswith(('.text', 'code', 'CODE')):
                continue
            if 'F' not in m.group(2) and not m.group(4).startswith(('_', 'c2p', 'pop2')):
                continue
            symbols.append((int(m.group(1), 16) - self.link[m.group(3)] + self.sections[m.group(3)][0], m.group(4).strip()))
        symbols.sort()
        named = {n: a for a, n in symbols}
        # GetTrapAddress veneers: each is $AFFE followed by the serviced trap word.
        self.cached = (named.get('pop2_cached_services'), named.get('pop2_cached_services_end'))
        # An end marker may alias the next function; it is not a call target.
        symbols = [(a, n) for a, n in symbols if n != "pop2_cached_services_end"
                   and not (n == "pop2_audio_fixture_end"
                            and a == named.get("pop2QuickHGetStateWarm"))]
        self.symbol_addrs = [a for a, _ in symbols]
        self.symbol_names = [s for _, s in symbols]
        self.cached_words = []
        links = dict((m.group(2), int(m.group(1), 16)) for m in
                     re.finditer(r'^([0-9a-f]{8}) .{7} \S+\s+[0-9a-f]{8} (pop2_cached_services(?:_end)?)$', table, re.M))
        if len(links) == 2:
            data = elf_section(elf, '.text')
            begin = links['pop2_cached_services'] - self.link['.text']
            data = data[begin:links['pop2_cached_services_end'] - self.link['.text']]
            self.cached_words = [int.from_bytes(data[i + 2:i + 4], 'big') for i in range(0, len(data), 4)]
        text = [v for k, v in self.sections.items() if k.startswith(('.text', 'code', 'CODE'))]
        self.text_range = (min(v[0] for v in text), max(v[1] for v in text))

    def _original_names(self):
        sys.path.insert(0, str(ROOT / 'tools'))
        import _amigaxdev  # noqa: F401  verifies the pinned shared helpers
        from amigaxdev.mac.resource_fork import read_resource_fork
        code = {r.rid: r.body for r in read_resource_fork(APP) if r.kind == b'CODE'}
        self.starts = {}
        for _, _, seg in self.segs:
            body = code[seg]
            self.starts[seg] = {i for i in range(4, len(body) - 1, 2) if body[i] == 0x4E and body[i + 1] == 0x56}
        self.names = {}
        records = collections.defaultdict(list)
        with (ROOT / 'disasm/symbols.csv').open() as f:
            for row in csv.reader(f):
                if len(row) < 3 or not row[0].isdigit():
                    continue
                seg, off, label = int(row[0]), int(row[1], 16), row[2]
                if label.endswith('_MacsBug'):
                    records[seg].append((off, label[:-8]))
                else:
                    self.names[(seg, off)] = label
        self.records = records

    def add_targets(self, nodes):
        for _, target, kind, _, _ in nodes.values():
            if kind == 0:
                target = self.jt.get(target, (0, target))[1]
                seg = self.segment(target)
                if seg:
                    self.starts[seg[2]].add(target - seg[0])

    def finish(self):
        self.seg_starts = {k: sorted(v) for k, v in self.starts.items()}
        # A MacsBug record follows its function's final RTS: name the last start before it.
        for seg, recs in self.records.items():
            starts = self.seg_starts.get(seg)
            if not starts:
                continue
            for off, label in recs:
                j = bisect.bisect_left(starts, off) - 1
                if j >= 0:
                    self.names.setdefault((seg, starts[j]), label)

    def segment(self, pc):
        i = bisect.bisect_right(self.seg_begins, pc) - 1
        if i >= 0 and self.segs[i][0] <= pc < self.segs[i][1]:
            return self.segs[i]
        return None

    def original(self, seg, off):
        name = self.names.get((seg, off))
        return f'CODE{seg}+${off:04X}' + (f' {name}' if name else '')

    def classify(self, pc):
        if pc in self.jt:
            region, fn = self.classify(self.jt[pc][1])
            return region, f'JT{self.jt[pc][0]}->{fn}'
        seg = self.segment(pc)
        if seg:
            starts = self.seg_starts[seg[2]]
            j = bisect.bisect_right(starts, pc - seg[0]) - 1
            return 'ORIG', self.original(seg[2], starts[j]) if j >= 0 else f'CODE{seg[2]}+?'
        if self.text_range[0] <= pc < self.text_range[1]:
            j = bisect.bisect_right(self.symbol_addrs, pc) - 1
            return 'NATIVE', self.symbol_names[j] if j >= 0 else 'native?'
        if 0xF80000 <= pc < 0x1000000:
            return 'ROM', f'ROM ${pc & ~0xFF:06X}'
        return 'OTHER', f'${pc:06X}'


class Phase:
    def __init__(self, ctx, path):
        self.ctx = ctx
        self.path = path
        self.header, self.pcs, self.nodes = parse_profile(path.read_text())
        self.children, self.order, self.inclusive = call_tree(self.nodes)
        self.units = self.header['units']
        self.units_per_ms = self.header['CYCLE_UNIT'] * self.header.get('colorclock_hz', 3546895) / 1000.0

    def label(self, n):
        parent, target, kind, _, _ = self.nodes[n]
        if n == 0:
            return '<root>'
        if kind == LINE_A:
            if target & 0xFFFF == 0xAFFE:
                return 'TRAP cached-service'
            return 'TRAP ' + trap_name(target & 0xFFFF, self.ctx.traps)
        begin, end = self.ctx.cached
        if kind == 0 and begin is not None and begin <= target < end:
            words = self.ctx.cached_words
            return 'CACHED ' + trap_name(words[(target - begin) // 4], self.ctx.traps)
        if kind:
            return f'EXC{kind} {short(self.ctx.classify(target)[1], 50)}'
        prefix = ''
        if target in self.ctx.jt:
            prefix = f'JT{self.ctx.jt[target][0]}->'
            target = self.ctx.jt[target][1]
        seg = self.ctx.segment(target)
        if seg:
            return prefix + self.ctx.original(seg[2], target - seg[0])
        return prefix + short(self.ctx.classify(target)[1], 70)


def load(name, phase):
    ctx = Context(name)
    paths = sorted(ctx.dir.glob('opening-*.prof'), key=lambda p: tuple(map(int, p.stem.split('-')[1:])))
    require(paths, 'no phase profiles (run with cycles)')
    if phase is not None:
        paths = [p for p in paths if int(p.stem.split('-')[1]) == phase]
        require(paths, f'no phase {phase}')
    phases = [Phase(ctx, p) for p in paths]
    for p in phases:
        ctx.add_targets(p.nodes)
    ctx.finish()
    for p in phases:
        p.labels = {n: p.label(n) for n in p.nodes}
    return ctx, phases


def scope(ctx, phases, phase):
    out, rows = summarize(ctx.dir.name)
    if phase is None:
        sel = out['phases']
    else:
        sel = [out['phases'][phase]]
    return sum(p['frames'] for p in sel), sum(p['fields'] for p in sel)


def linea_report(phases):
    if all(p.header.get('executed_pc_frozen') == 1 for p in phases):
        from native_service_scope import linea
        cost=sum(sum(linea(p.header,p.pcs,p.nodes).values())/p.units_per_ms for p in phases)
        print(f'CPU Line-A entry: {cost:.6f} ms in all captured parts (already included in PC/tree totals)')


def report(name, phase, top):
    ctx, phases = load(name, phase)
    linea_report(phases)
    frames_n, fields = scope(ctx, phases, phase)
    if ctx.config.get('active', '0') == '1':
        require('ACTIVE SCOPE COMPLETE' in ctx.log, 'INCOMPLETE ACTIVE CAPTURE')
        print('ACTIVE WORK: excludes WaitTimerNoPoll entry through UNLK; includes its RTS; full-scene frame denominator')
    units = sum(p.units for p in phases)
    upm = phases[0].units_per_ms
    gap = sum(p.header['gap'] for p in phases)
    ms = lambda u: u / upm / frames_n
    pct = lambda u: 100.0 * u / units
    total = units / upm
    print(f"== {name} phase {'all' if phase is None else phase} ({ctx.config['setup']}, cycles at {ctx.hz} Hz): "
          f'{frames_n} frames, {fields} fields, {total:.1f} ms emulated CPU, {total / frames_n:.2f} ms/frame '
          f'({fields / frames_n:.2f} fields/frame)')
    if ctx.config.get('active', '0') == '1':
        wait_ids = [int(n) for n, ph in re.findall(r'^ACTIVE WAIT ENTER (\d+) phase=(\d+) ', ctx.log, re.M)
                    if phase is None or int(ph) == phase]
        waits = [Phase(ctx, ctx.dir / f'wait-{n}.prof') for n in wait_ids]
        require(waits or phase is not None, 'MISSING WAIT ACCOUNTING')
        wait_ms = sum(p.units for p in waits) / upm
        print(f'   Accounting: active {total:.3f} ms + wait {wait_ms:.3f} ms = {total + wait_ms:.3f} ms; '
              f'{(total + wait_ms) / frames_n:.3f} ms/full frame; {len(phases)} active parts, {len(waits)} waits')
    print(f"   Unassigned boundary: {sum(p.header['tail'] for p in phases) / upm:.6f} ms total (reported separately)")
    h = phases[0].header
    print(f"   CACR=${h.get('cacr', 0):X} cpu_model {h.get('cpu_model')}; unmatched RTS "
          f"{sum(p.header['unmatched_rts'] for p in phases)}, RTE {sum(p.header['unmatched_rte'] for p in phases)}, "
          f"overflows {sum(p.header['overflow'] for p in phases)}; {len(ctx.jt)} jump-table entries")
    flat = collections.Counter(); execs = collections.Counter(); buckets = collections.Counter()
    chip_u = collections.Counter(); chip_n = collections.Counter()
    for p in phases:
        for pc, (u, k, cu, cn) in p.pcs.items():
            key = ctx.classify(pc)
            flat[key] += u; execs[key] += k; buckets[bucket(*key)] += u
            if cn:
                chip_u[key] += cu; chip_n[key] += cn
    buckets['Exception entry and STOP gaps'] += gap
    buckets['Unassigned capture boundary'] += sum(p.header['tail'] for p in phases)
    buckets['Unmapped PC'] += sum(p.header.get('high', 0) for p in phases)
    print('\n-- Exclusive time by bucket, ms per frame --')
    for b, u in buckets.most_common():
        print(f'{pct(u):6.2f}%  {ms(u):7.2f} ms  {b}')
    clock_names = {'pop2WallTicks', 'pop2ServiceTicks', 'anon::timerNow', 'anon::timerDueNow'}
    clock_units = 0
    for p in phases:
        for n in p.order:
            if p.labels[n] not in clock_names:
                continue
            ancestor = p.nodes[n][0]
            while ancestor and p.labels[ancestor] not in clock_names:
                ancestor = p.nodes[ancestor][0]
            if not ancestor:
                clock_units += p.inclusive[n]
    print(f'\nClock sampling (WallTicks + ServiceTicks + named Time Manager queries, inclusive, no nested double count): {ms(clock_units):.6f} ms/frame')
    if not any(p.labels[n] in {'anon::timerNow', 'anon::timerDueNow'} for p in phases for n in p.order):
        print('No separately attributed Time Manager query in this capture; older builds inline those reads.')
    print('Clock sampling overlaps the existing function buckets; do not add it to their total.')
    if chip_n:
        cu = sum(chip_u.values()); cn = sum(chip_n.values())
        print(f'\n-- CPU Chip-bus accesses: {pct(cu):.1f}% of time, {ms(cu):.2f} ms/frame, {cn / frames_n:.0f} accesses/frame, '
              f'{cu / upm / 1000 * ctx.hz / cn:.1f} CPU cycles/access --')
        for key, c in chip_u.most_common(10):
            print(f'{pct(c):6.2f}%  {ms(c):7.2f} ms  {chip_n[key] / frames_n:8.0f} accesses  {key[0]:<6} {short(key[1], 60)}')
    print(f'\n-- Exclusive time by function, top {top} --')
    for key, u in flat.most_common(top):
        print(f'{pct(u):6.2f}%  {ms(u):7.2f} ms  {execs[key] / frames_n:9.0f} instr/frame  {key[0]:<6} {short(key[1])}')
    included = collections.Counter(); calls = collections.Counter()
    traps = collections.Counter(); trap_calls = collections.Counter()
    for p in phases:
        for n in p.order[1:]:
            label = p.labels[n]
            calls[label] += p.nodes[n][4]
            a = p.nodes[n][0]
            while a and p.labels[a] != label:
                a = p.nodes[a][0]
            if not a:
                included[label] += p.inclusive[n]
            if p.nodes[n][2] == LINE_A:
                a = p.nodes[n][0]
                while a and p.nodes[a][2] != LINE_A:
                    a = p.nodes[a][0]
                if not a:
                    traps[label] += p.inclusive[n]; trap_calls[label] += p.nodes[n][4]
    print(f'\n-- Inclusive time by call target, top {top} --')
    for label, u in included.most_common(top):
        print(f'{pct(u):6.2f}%  {ms(u):7.2f} ms  {calls[label] / frames_n:8.1f} calls/frame  {label}')
    print(f'\n-- Outermost Toolbox traps, inclusive ({pct(sum(traps.values())):.1f}% of time) --')
    for label, u in traps.most_common(30):
        per = u / max(1, trap_calls[label]) / upm * 1000
        print(f'{pct(u):6.2f}%  {ms(u):7.2f} ms  {trap_calls[label] / frames_n:8.1f} calls/frame  {per:8.1f} us/call  {label}')
    suffix = 'all' if phase is None else str(phase)
    with (ctx.dir / f'tree-{suffix}.txt').open('w') as f:
        for p in phases:
            f.write(f'### {p.path.name}\n')
            def dump(n, depth):
                if p.inclusive[n] < p.units * 0.002 or depth > 40:
                    return
                f.write(f'{"  " * depth}{100.0 * p.inclusive[n] / p.units:6.2f}% (self {100.0 * p.nodes[n][3] / p.units:5.2f}%) '
                        f'{p.nodes[n][4]:8d} calls  {p.labels[n]}\n')
                for c in sorted(p.children[n], key=lambda c: -p.inclusive[c]):
                    dump(c, depth + 1)
            dump(0, 0)
    merged = collections.Counter(); mexec = collections.Counter(); mchip = collections.Counter()
    for p in phases:
        for pc, (u, k, cu, cn) in p.pcs.items():
            merged[pc] += u; mexec[pc] += k; mchip[pc] += cn
    with (ctx.dir / f'hot-{suffix}.txt').open('w') as f:
        for pc, u in merged.most_common(500):
            region, fn = ctx.classify(pc)
            seg = ctx.segment(pc)
            where = f'CODE{seg[2]}+${pc - seg[0]:04X}' if seg else ''
            f.write(f'{pc:06x} {pct(u):6.3f}% {mexec[pc]:10d}x {u / upm / 1000 * ctx.hz / max(1, mexec[pc]):7.1f} cyc  '
                    f'chip {mchip[pc]:7d}  {region:<6} {where:<14} {short(fn, 60)}\n')
    print(f'\nwrote tree-{suffix}.txt and hot-{suffix}.txt in {ctx.dir.relative_to(ROOT)}')


def annotate(name, function, phase, top):
    ctx, phases = load(name, phase)
    text = subprocess.run([OBJDUMP, '-d', '-C', str(ctx.dir / 'PrinceOfPersia2.elf')],
                          capture_output=True, text=True, check=True).stdout
    instructions, section, current = {}, None, None
    for line in text.splitlines():
        m = re.match(r'Disassembly of section (\S+):', line)
        if m:
            section = m.group(1); continue
        m = re.match(r'[0-9a-f]+ <(.*)>:', line)
        if m:
            current = m.group(1); continue
        m = re.match(r'\s*([0-9a-f]+):\s+(?:[0-9a-f]{4} )+\s*(.*)', line)
        if m and current and function in current and section in ctx.sections:
            instructions[int(m.group(1), 16) - ctx.link[section] + ctx.sections[section][0]] = m.group(2).strip()
    require(instructions, f'no native function matches {function}')
    units = sum(p.units for p in phases); upm = phases[0].units_per_ms
    merged = collections.defaultdict(lambda: [0, 0, 0])
    for p in phases:
        for pc in instructions:
            if pc in p.pcs:
                u, k, _, cn = p.pcs[pc]
                merged[pc][0] += u; merged[pc][1] += k; merged[pc][2] += cn
    rows = sorted(((merged[pc], pc, ins) for pc, ins in instructions.items()), key=lambda r: -r[0][0])
    print(f'{function}: {100.0 * sum(r[0][0] for r in rows) / units:.2f}% of the interval')
    for (u, k, cn), pc, ins in rows[:top]:
        print(f'{pc:06x} {100.0 * u / units:6.3f}% {k:9d}x {u / upm / 1000 * ctx.hz / max(1, k):6.1f} cyc  chip {cn:7d}  {ins}')


def calls(name, label, parent, phase):
    ctx, phases = load(name, phase)
    frames_n, _ = scope(ctx, phases, phase)
    count = units = 0
    for p in phases:
        for n in p.order:
            if p.labels[n] == label and (parent is None or p.labels[p.nodes[n][0]] == parent):
                count += p.nodes[n][4]; units += p.inclusive[n]
    require(count, 'no matching calls')
    upm = phases[0].units_per_ms
    print(f'{label} under {parent or "all callers"}: {count} calls, {units / upm * 1000 / count:.2f} us/call, '
          f'{units / upm / frames_n:.3f} ms/frame (inclusive)')


def subtree(name, labels, phase):
    ctx, phases = load(name, phase)
    frames_n, _ = scope(ctx, phases, phase)
    upm = phases[0].units_per_ms
    units = sum(p.units for p in phases)
    for want in labels:
        original = port = 0
        for p in phases:
            for n in p.nodes:
                if p.labels[n] != want:
                    continue
                stack = [n]
                while stack:
                    x = stack.pop(); stack.extend(p.children[x])
                    _, target, kind, self_units, _ = p.nodes[x]
                    target = ctx.jt.get(target, (0, target))[1]
                    if not kind and ctx.segment(target):
                        original += self_units
                    else:
                        port += self_units
        print(f'{want:<40} original {original / upm / frames_n:6.2f} ms  port {port / upm / frames_n:6.2f} ms  '
              f'total {(original + port) / upm / frames_n:6.2f} ms/frame ({100.0 * (original + port) / units:.1f}%)')


def work(name, phase, excludes, top):
    """Cost outside named call subtrees (default: the original's timer wait loop).

    Node self-time is attributed to each call-tree node's entry label; tail jumps
    and interrupts make this approximate, unlike the exact flat PC report."""
    ctx, phases = load(name, phase)
    linea_report(phases)
    frames_n, fields = scope(ctx, phases, phase)
    upm = phases[0].units_per_ms
    units = sum(p.units for p in phases)
    kept = 0
    selfs = collections.Counter(); traps = collections.Counter(); trap_calls = collections.Counter()
    for p in phases:
        excluded = set()
        for n in p.order:
            plain = re.sub(r'^JT\d+->', '', p.labels[n])
            if any(plain.startswith(e) or p.labels[n].startswith(e) for e in excludes) or (n and p.nodes[n][0] in excluded):
                excluded.add(n)
        for n in p.order:
            if n in excluded:
                continue
            kept += p.nodes[n][3]
            selfs[p.labels[n]] += p.nodes[n][3]
            if p.nodes[n][2] == LINE_A:
                a = p.nodes[n][0]
                while a and p.nodes[a][2] != LINE_A:
                    a = p.nodes[a][0]
                if not a:
                    traps[p.labels[n]] += p.inclusive[n]; trap_calls[p.labels[n]] += p.nodes[n][4]
        kept += 0
    gap = sum(p.header['gap'] for p in phases)
    ms = lambda u: u / upm / frames_n
    print(f"== {name} phase {'all' if phase is None else phase}: {frames_n} frames, {fields / frames_n:.2f} fields/frame; "
          f'total {ms(units):.2f} ms/frame, outside {excludes}: {ms(kept):.2f} ms/frame '
          f'(+{ms(gap):.2f} ms exception/STOP gaps, unattributed)')
    print(f'\n-- Outermost traps outside the excluded subtrees ({ms(sum(traps.values())):.2f} ms/frame) --')
    for label, u in traps.most_common(top):
        print(f'{ms(u):7.2f} ms  {trap_calls[label] / frames_n:7.1f} calls/frame  {u / max(1, trap_calls[label]) / upm * 1000:8.1f} us/call  {label}')
    print(f'\n-- Node self time outside the excluded subtrees, top {top} --')
    for label, u in selfs.most_common(top):
        print(f'{ms(u):7.2f} ms  {label}')


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest='command', required=True)
    c = sub.add_parser('check-run'); c.add_argument('run', type=Path)
    o = sub.add_parser('opening'); o.add_argument('name'); o.add_argument('--compare'); o.add_argument('--original', help='original no-input reference run')
    r = sub.add_parser('report'); r.add_argument('name'); r.add_argument('--phase', type=int); r.add_argument('--top', type=int, default=40)
    a = sub.add_parser('annotate'); a.add_argument('name'); a.add_argument('function'); a.add_argument('--phase', type=int); a.add_argument('--top', type=int, default=25)
    k = sub.add_parser('calls'); k.add_argument('name'); k.add_argument('label'); k.add_argument('--parent'); k.add_argument('--phase', type=int)
    s = sub.add_parser('subtree'); s.add_argument('name'); s.add_argument('labels', nargs='+'); s.add_argument('--phase', type=int)
    wk = sub.add_parser('work'); wk.add_argument('name'); wk.add_argument('--phase', type=int); wk.add_argument('--top', type=int, default=25)
    wk.add_argument('--exclude', action='append')
    args = ap.parse_args()
    if args.command == 'check-run':
        check_run(args.run)
    elif args.command == 'opening':
        opening(args.name, args.compare, args.original)
    elif args.command == 'report':
        report(args.name, args.phase, args.top)
    elif args.command == 'annotate':
        annotate(args.name, args.function, args.phase, args.top)
    elif args.command == 'work':
        work(args.name, args.phase, args.exclude or ['CODE17+$31B8'], args.top)
    elif args.command == 'calls':
        calls(args.name, args.label, args.parent, args.phase)
    else:
        subtree(args.name, args.labels, args.phase)


if __name__ == '__main__':
    main()
