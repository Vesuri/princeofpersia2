#!/usr/bin/env python3
"""Reproduce the source inventory from the pinned archive; no reference run."""
import re
import struct
from collections import Counter, defaultdict

from extract_original_data import (APPLICATION, ARCHIVE, unpack,
                                   scratch_directory, verify_archive, verify_output)
from amigaxdev.mac.resource_fork import read_resource_fork, find_resource
from amigaxdev.mac import m68k_sweep


def require(condition, label):
    if not condition:
        raise ValueError('INSPECT / ' + label)


def strings(data):
    return [s.decode('mac_roman') for s in re.findall(rb'[\x20-\x7e\xa0-\xff]{4,}', data)]


def procedure_names(resources):
    """Conservative subset: length-prefixed MacsBug records immediately after RTS.

    Accept both $80,length and $80+length forms, including a trailing NUL in
    the length. An unrestricted printable-string scan also finds code bytes.
    Offsets are relative to the original CODE resource, including its header.
    """
    result = []
    for r in resources:
        if r.kind != b'CODE' or r.rid == 0:
            continue
        data = r.body
        for i in range(4, len(data) - 2, 2):
            if data[i-2:i] != b'\x4e\x75':
                continue
            if data[i] == 128:
                size, start = data[i+1], i+2
            elif 128 < data[i] < 160:
                size, start = data[i]-128, i+1
            else:
                continue
            name = data[start:start+size].rstrip(b'\0')
            if start+size <= len(data) and re.fullmatch(rb'[A-Za-z_][A-Za-z0-9_.$%:]*', name):
                result.append((r.rid, i, name.decode('ascii')))
    return result


def main():
    digest = verify_archive(ARCHIVE)
    print(f'MEASURED archive: {ARCHIVE.name}, {ARCHIVE.stat().st_size} bytes, SHA-256 {digest}')
    scratch = scratch_directory()
    root, groups = unpack(ARCHIVE, scratch)
    verify_output(root)
    print('MEASURED format: StuffIt 5; application + Data/ (31 files); all output hashes verified')
    app = root / APPLICATION
    info = app.with_name(APPLICATION + '.finfo').read_bytes()
    record = groups[APPLICATION][0]
    print('MEASURED application:', APPLICATION, 'type', info[4:8].decode('mac_roman'),
          'creator', info[8:12].decode('mac_roman'), 'modified', record['XADLastModificationDate'])
    require(app.stat().st_size == 0, 'APPLICATION DATA FORK')
    require(info[4:12] == b'APPLPRNc', 'APPLICATION FINDER IDENTITY')
    rs = read_resource_fork(root / (APPLICATION + '.rsrc'))
    counts = Counter(r.kind for r in rs)
    get = lambda kind, rid: find_resource(rs, kind, rid).body
    version = get(b'vers', 1)
    print('MEASURED vers 1:', version[7:7+version[6]].decode('mac_roman'))
    require(version[7:7+version[6]] == b'1.0', 'VERSION')
    require(not counts[b'cfrg'] and not counts[b'CREL'], 'ARCHITECTURE/MODEL')
    print('MEASURED architecture: no cfrg; no CREL')
    code = {r.rid: r for r in rs if r.kind == b'CODE'}
    require(set(code) == set(range(24)), 'CODE SEGMENT SET')
    entries, header, clean = m68k_sweep.jump_table(code[0].body)
    above, below, size, offset = header
    require(clean and len(entries)*8 == size and len(code[0].body) == 16+size, 'UNLOADED JUMP TABLE')
    require(header == (8176, 22266, 8144, 32), 'A5 WORLD')
    print(f'MEASURED A5: {above} above, {below} below; {len(entries)} unloaded entries at A5+{offset}')
    roots = defaultdict(set)
    for segment, entry in entries:
        require(segment in code and 0 <= entry < len(code[segment].body)-4, 'JUMP TABLE TARGET')
        roots[segment].add(entry)
    require(m68k_sweep.selftest() == 0, 'CPU CLASSIFIER CONTROL')
    found, reached, total, segments, blind_total = 0, 0, 0, [], 0
    for rid, r in sorted(code.items()):
        if rid == 0:
            continue
        first, count = struct.unpack_from('>HH', r.body)
        require(first != 0xffff, 'NEAR SEGMENT HEADER')
        findings, seen, blind = m68k_sweep.walk(r.body[4:], sorted(roots[rid]), f'CODE {rid}')
        print(f'MEASURED CODE {rid}: {len(r.body)} bytes, {r.name!r}; '
              f'68020={len(findings)}, reached={len(seen)}/{len(r.body)-4}, unresolved transfers={blind}')
        found += len(findings)
        reached += len(seen)
        total += len(r.body)-4
        blind_total += blind
        if findings:
            segments.append(rid)
    require(all(len(code[i].body) == 4 for i in (12, 13, 14)), 'EDITOR STUBS')
    require(found == 216 and segments == [5, 11, 16, 17, 21, 22], 'CPU FINDINGS')
    require(f'{100*reached/total:.1f}' == '74.3', 'CPU COVERAGE')
    print(f'MEASURED CPU: {found} 68020-only candidates on statically followed paths; '
          f'{100*reached/total:.1f}% coverage; {blind_total} unresolved transfers. Not execution evidence.')
    names = procedure_names(rs)
    print(f'MEASURED MacsBug: {len(names)} RTS-adjacent name records (conservative subset)')
    require(len(names) == 1032, 'MACSBUG RECORD COUNT')
    for wanted in ('CopyProtectionDialog', 'PackAllData', 'LZPackDataFile', 'PackDataAtStartUp', 'DecryptHandle'):
        matches = [(seg, off) for seg, off, name in names if name == wanted]
        require(bool(matches), 'MISSING NAME: ' + wanted)
        print('MEASURED name', wanted, matches, '(CODE ID, resource offset)')
    for literal in (b'Jnth', b'MDRV'):
        locations = [(r.rid, m.start()) for r in code.values()
                     for m in re.finditer(re.escape(literal), r.body)]
        require(bool(locations), 'SOUND RESOURCE TYPE LITERAL')
        print('MEASURED sound type literal', literal.decode(), locations,
              '(CODE ID, resource offset; request semantics need code inspection)')
    for kind in (b'DATA', b'ZERO', b'DREL', b'_STD', b'_STI', b'STRS'):
        items = [(r.rid, len(r.body)) for r in rs if r.kind == kind]
        require(bool(items), 'RUNTIME RESOURCE ' + str(kind))
        print('MEASURED runtime resource', kind.decode(), items)
    require(len(code[1].body) == 1058, 'CODE 1 SIZE')
    print('INFERRED runtime: THINK/Symantec family; not established by execution')
    for r in rs:
        if r.kind == b'SIZE':
            flags, preferred, minimum = struct.unpack('>HII', r.body)
            print(f'MEASURED SIZE {r.rid}: flags ${flags:04X}, minimum {minimum}, preferred {preferred}')
    require(struct.unpack('>HII', get(b'SIZE', 0)) == (0x4a80, 3072000, 2560000), 'SIZE 0')
    top, left, bottom, right, proc = struct.unpack_from('>hhhhh', get(b'WIND', 400))
    require((right-left, bottom-top, proc) == (512, 384, 4), 'GAME WINDOW')
    print(f'MEASURED WIND 400: {right-left}x{bottom-top}, procID {proc}')
    for rid, terms in ((402, ('6.0.7', '256 colors', '32 Bit QuickDraw')),
                       (602, ('Virtual Memory', 'OFF')),
                       (600, ('1993, 1994', 'Jordan Mechner', 'Brøderbund'))):
        text = get(b'DITL', rid).decode('mac_roman')
        require(all(term in text for term in terms), 'DITL TEXT ' + str(rid))
    require(b'Compress Data' in get(b'MENU', 403), 'PACKING MENU')
    for r in rs:
        if r.kind in (b'DITL', b'MENU'):
            print('MEASURED text', r.kind.decode(), r.rid, strings(r.body))
    for kind in (b'MDRV', b'SMOD', b'INST', b'snd '):
        items = [(r.rid, r.name) for r in rs if r.kind == kind]
        print('MEASURED sound', kind.decode(), len(items), items if kind in (b'MDRV', b'SMOD') else '')
    require(find_resource(rs, b'MDRV', 11).name == 'MIDI Synth 3.45 4/27/94', 'MDRV IDENTITY')
    require({r.rid for r in rs if r.kind == b'SMOD'} == {0, 1, 2, 3}, 'SMOD IDS')
    require(counts[b'INST'] == counts[b'snd '] == 34, 'SOUND RESOURCE COUNTS')
    prince = read_resource_fork(root / 'Data/Prince.rsrc.rsrc')
    require(any(r.kind == b'COPY' for r in prince), 'COPY RESOURCE')
    print('MEASURED Data/Prince.rsrc: COPY resource present')
    print('INSPECT PASS: archive, output hashes and application checks completed')


if __name__ == '__main__':
    main()
