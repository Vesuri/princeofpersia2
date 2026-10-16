"""Replay a pinned assisted reference; interventions are never exit evidence."""
import hashlib
import re
from pathlib import Path
from extract_original_data import ROOT, pins

REFERENCE = ROOT / 'ref/mame/runs/gameplay-assisted-survey-3'
MANIFEST = ROOT / 'manifest/gameplay-assisted-survey.sha256'
ALLOWED = {0x56d8, 0x4fce, 0x4fcc, 0x4fca, 0x4fa2, 0x50ca, 0x509c, 0x50b4, 0x50b2, 0x50ce, 0x4faa, 0x4f92, 0x4fac, 0x50bc, 0x50cc, 0x50c4, 0x4fc2, 0x5244}


def metadata(reference=REFERENCE, manifest=MANIFEST):
    for name, expected in pins(manifest).items():
        assert hashlib.sha256((reference / name).read_bytes()).hexdigest() == expected, 'ASSIST INPUT / HASH ' + name
    text = (reference / 'session.log').read_text()
    ending = text.count('GAMEPLAY_ENDING_COMPLETE\n') == 1
    expired = text.count('GAMEPLAY_EXPIRED_COMPLETE\n') == 1
    assert not (ending and expired), 'ASSIST INPUT / TERMINAL OWNER'
    assert 'FAIL REFERENCE' not in text and (ending or expired or text.count('GAMEPLAY_DISCOVERY_COMPLETE\n') == 1), 'ASSIST INPUT / COMPLETION'
    if expired:
        assert text.count('GAMEPLAY_EXPIRED_RETURN result=1\n') == 1, 'ASSIST INPUT / EXPIRED RESULT'
        assert text.count('GAMEPLAY_ROUTE_NIS_ENTER selector=28\n') == text.count('GAMEPLAY_ROUTE_NIS_RETURN selector=28\n') == 1, 'ASSIST INPUT / EXPIRED RETURN'
        assert len(re.findall(r'^CUTSCENE_RETURN ordinal=\d+ selector=20 result=0 copy=\d+ text=\d+$',text,re.M)) == 1, 'ASSIST INPUT / EXPIRED CINEMATIC'
    if ending:
        assert text.count('GAMEPLAY_VICTORY result=15\n') == 1, 'ASSIST INPUT / VICTORY'
        assert text.count('GAMEPLAY_ROUTE_NIS_ENTER selector=11\n') == text.count('GAMEPLAY_ROUTE_NIS_RETURN selector=11\n') == 1, 'ASSIST INPUT / ENDING RETURN'
    keys, points, writes, labels = [], [], [], []
    pending = []
    ending_points = 0
    for line in text.splitlines():
        key = re.fullmatch(r'GAMEPLAY_ROUTE_KEYS frame=(\d+) step=(\d+) map=([0-9A-F]{32})', line)
        if key:
            keys.append(key.groups())
        write = re.fullmatch(r'GAMEPLAY_ASSIST sequence=(\d+) kind=(\w+) a5minus=([0-9A-F]+) before=([0-9A-F]+) after=([0-9A-F]+)', line)
        if write:
            sequence, kind, offset, before, after = write.groups()
            assert int(offset,16) in ALLOWED and kind in ('health','level','x','room','y','shadow','minutes','protection'), 'ASSIST INPUT / WRITE'
            if kind == 'protection' or int(offset,16) == 0x56d8:
                assert kind == 'protection' and (int(offset,16),int(before,16),int(after,16)) == (0x56d8,0,1), 'ASSIST INPUT / D6 PROTECTION SEAM'
            pending.append((int(sequence), int(offset,16), int(before,16), int(after,16)))
        if not line.startswith('GAMEPLAY_ROUTE_CHECKPOINT '):
            continue
        row = dict(v.split('=') for v in line.split()[1:])
        reason = row['reason']
        if reason in ('ending-return','expired-return'):ending_points += 1
        if reason in ('discovery-finished', 'victory'):
            continue
        assert reason in ('first-frame','next-level','completed-frames','assist-before','assisted','ending-return','expired-return'), 'ASSIST INPUT / CHECKPOINT'
        assert int(row['frames']) == int(row['step']), 'ASSIST INPUT / FRAME STEP'
        ordinal = len(points)
        if reason == 'assisted':
            assert pending and all(w[0] == int(row['sequence']) for w in pending), 'ASSIST INPUT / WRITE OWNER'
            assert labels[-1] == row['prefix']+'-before', 'ASSIST INPUT / BEFORE CAPTURE'
            writes.extend((ordinal,*w[1:]) for w in pending)
            pending = []
        else:
            assert not pending, 'ASSIST INPUT / UNCONSUMED WRITES'
        points.append((int(row['step']),int(row['level']),int(row['room']),len(keys),reason in ('first-frame','next-level'),reason in ('assist-before','assisted')))
        assert re.fullmatch(r'route-[0-9]+(?:-before)?',row['prefix']), 'ASSIST INPUT / CAPTURE NAME'
        labels.append(row['prefix'])
    assert not pending and points and points[0][:4] == (0,1,4,0), 'ASSIST INPUT / INITIAL STATE'
    assert ending_points == int(ending or expired), 'ASSIST INPUT / ENDING CHECKPOINT'
    if expired:
        assert row['reason']=='expired-return', 'ASSIST INPUT / EXPIRED FINAL STATE'
        final_a5=(reference/(labels[-1]+'-a5.bin')).read_bytes()
        assert final_a5[-0x4fce:-0x4fcc]==bytes(2), 'ASSIST INPUT / EXPIRED MINUTES'
    if ending:
        assert row['reason']=='ending-return' and points[-1][1]==14, 'ASSIST INPUT / ENDING FINAL STATE'
    assert len(set(labels)) == len(labels), 'ASSIST INPUT / DUPLICATE CAPTURE'
    for label in labels:
        for suffix,size in (('-a5.bin',22266),('-context.bin',0x2200),('.indices',307200),('.rgb',768)):
            assert (reference/(label+suffix)).stat().st_size == size, 'ASSIST INPUT / CAPTURE EXTENT'
    for ordinal in sorted(set(w[0] for w in writes)):
        before = (reference/(labels[ordinal-1]+'-a5.bin')).read_bytes()
        expected = bytearray(before)
        for _,offset,old,new in (w for w in writes if w[0] == ordinal):
            assert int.from_bytes(expected[-offset:-offset+2],'big') == old, 'ASSIST INPUT / BEFORE VALUE'
            expected[-offset:-offset+2] = new.to_bytes(2,'big')
        assert bytes(expected) == (reference/(labels[ordinal]+'-a5.bin')).read_bytes(), 'ASSIST INPUT / AFTER VALUE'
        for suffix in ('-context.bin','.indices','.rgb'):
            assert (reference/(labels[ordinal-1]+suffix)).read_bytes() == (reference/(labels[ordinal]+suffix)).read_bytes(), 'ASSIST INPUT / UNLOGGED CHANGE'
    return keys,points,writes,labels


def generate(reference=REFERENCE, manifest=MANIFEST):
    keys,points,writes,_ = metadata(reference, manifest)
    ending = 'GAMEPLAY_ENDING_COMPLETE\n' in (reference/'session.log').read_text()
    expired = 'GAMEPLAY_EXPIRED_COMPLETE\n' in (reference/'session.log').read_text()
    from gameplay_route_inputs import key_rows
    out=['// PRIVATE: pinned assisted reference input; ignored build/ only.',
         '#if POP2_GAMEPLAY_ROUTE_LEVEL != 0', '#error Assisted route requires level zero', '#endif',
         'struct RouteKey { uint16_t step; uint8_t mask; uint8_t keys[16]; };',
         'static const RouteKey routeKeys[] = {',*key_rows(keys),'};',
         'struct RouteCheckpoint { uint16_t step,level,room,keys; bool first,automatic; };',
         'static const RouteCheckpoint routeCheckpoints[] = {']
    out += ['{%d,%d,%d,%d,%s,%s},' % (*p[:4],str(p[4]).lower(),str(p[5]).lower()) for p in points]
    out += ['};','struct RouteAssist { uint16_t ordinal,offset,before,after; };',
            'static const RouteAssist routeAssists[] = {']
    out += ['{%d,%d,%d,%d},' % w for w in writes]
    out += ['};','static const char routeCompleteReason[] = "GAMEPLAY ROUTE / ASSISTED SURVEY COMPLETE";']
    out += [f'static const int routeEndingOrdinal = {len(points)-1 if ending or expired else -1};',
            f'static const bool routeExpired = {str(expired).lower()};']
    if ending:out = [line.replace('ASSISTED SURVEY COMPLETE','ASSISTED ENDING COMPLETE') for line in out]
    if expired:out = [line.replace('ASSISTED SURVEY COMPLETE','EXPIRED STORY COMPLETE') for line in out]
    return '\n'.join(out)+'\n'
