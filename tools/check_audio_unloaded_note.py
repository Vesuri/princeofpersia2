"""Pair unloaded-note admission with original loaded/reloaded controls."""
import argparse
import hashlib
from pathlib import Path
import re
import struct
import subprocess
from extract_original_data import ROOT, DESTINATION, pins, verify_output
from audio_inputs import content
import trap_census as census


def check(reference):
    for name, digest in pins(ROOT/'manifest/audio-unloaded-note.sha256').items():
        assert hashlib.sha256((reference/name).read_bytes()).hexdigest() == digest, 'UNLOADED NOTE / PIN '+name
    log = (reference/'session.log').read_text()
    assert log.count('AUDIO_UNLOADED_NOTE_COMPLETE calls=8\n') == 1 and 'FAIL REFERENCE' not in log, 'UNLOADED NOTE / COMPLETION'
    assets = content()
    assert (reference/'audio-driver.bin').read_bytes() == assets['driver-expanded.bin'], 'UNLOADED NOTE / DRIVER'
    rows = re.findall(r'AUDIO_FIXTURE_RESULT index=(\d+) selector=(\d+) note=(\d+) d0=([0-9A-F]{8})', log)
    assert [(int(i), int(s), int(n), int(r,16)) for i,s,n,r in rows] == [
        (1,6,0,0),(2,34,0,0),(3,9,60,0),(4,6,0,0),
        (5,36,0,0),(6,9,61,0),(7,34,0,0),(8,9,62,0)], 'UNLOADED NOTE / RESULTS'
    def state(side, index):
        b = (reference/f'audio-fixture-{side}-{index}.bin').read_bytes()
        assert len(b) == 0x32f0, 'UNLOADED NOTE / EXTENT'
        return b
    expected = []
    for index in (3,6,8):
        before, after = state('before',index), state('after',index)
        loaded = int.from_bytes(before[0x72:0x76],'big') != 0
        assert loaded == (index != 6), 'UNLOADED NOTE / LOAD CONTROL'
        durations = [struct.unpack_from('>h',after,0x26f2+4*i)[0] for i in range(5)]
        if index == 6:
            assert before[0x24f2:0x2ed4] == after[0x24f2:0x2ed4], 'UNLOADED NOTE / VOICE MUTATION'
            assert durations == [-1]*5, 'UNLOADED NOTE / ALLOCATED'
            expected.append((index,-1,0))
        else:
            assert durations == [20000,-1,-1,-1,-1], 'UNLOADED NOTE / POSITIVE CONTROL'
            expected.append((index,0,int.from_bytes(after[0x2532:0x2536],'big')))
    verify_output(DESTINATION);census.load(DESTINATION/'Prince of Persia 2.rsrc')
    data = bytearray(b'HAP1'+assets['audio-pitch.bin'])
    for i in range(34):
        inst = census.R[b'INST',i].body
        sample = census.R[b'snd ',int.from_bytes(inst[:2],'big')].body
        data += inst + struct.pack('>I',len(sample)) + sample
    # Host engine starts uninitialized; original fixture uses the game's driver.
    first = state('before',1)
    counts = [int.from_bytes(first[o:o+2],'big') for o in (0x13d6,0x13d8,0x13da)]
    events = [(21,0,*counts,0,0)]
    for _, selector, note, _ in rows:
        selector, note = int(selector), int(note)
        events.append((selector,0,0,note,20000 if selector==9 else 0,80 if selector==9 else 0,0))
    data += struct.pack('>I',len(events))
    for event in events:data += struct.pack('>8I',0,*event)
    out = ROOT/'tmp/audio-unloaded-note';out.mkdir(exist_ok=True)
    exe = out/'probe'
    subprocess.run(['c++','-std=c++17','-g','-fsanitize=address,undefined','-Isrc','tools/probe_audio_events.cpp','-o',str(exe)],cwd=ROOT,check=True)
    result = subprocess.run([str(exe)],input=data,stdout=subprocess.PIPE,check=True)
    actual = [tuple(map(int,line.split()[1:])) for line in result.stdout.splitlines() if line.startswith(b'N ')]
    assert actual == expected, ('UNLOADED NOTE / PAIR',actual,expected)
    print('UNLOADED NOTE PAIR PASS: original no-voice result and loaded/reloaded admission/pitch controls')


def check_route(reference):
    for name, digest in pins(ROOT/'manifest/audio-unloaded-note-live.sha256').items():
        assert hashlib.sha256((reference/name).read_bytes()).hexdigest() == digest, 'UNLOADED NOTE LIVE / PIN'
    log = (reference/'session.log').read_text()
    assert log.count('AUDIO_OBSERVER_COMPLETE\n') == log.count('GAMEPLAY_DISCOVERY_COMPLETE\n') == 1, 'UNLOADED NOTE LIVE / COMPLETION'
    assert 'FAIL REFERENCE' not in log, 'UNLOADED NOTE LIVE / FAILURE'
    assert (reference/'audio-driver.bin').read_bytes() == content()['driver-expanded.bin'], 'UNLOADED NOTE LIVE / DRIVER'
    loaded, stack, missing, step = set(), [], [], 0
    for line in log.splitlines():
        if line.startswith('GAMEPLAY_ROUTE_KEYS '):step=int(re.search(r'step=(\d+)',line)[1])
        elif line.startswith('AUDIO_CALL '):
            fields={k:int(v,16) for k,v in re.findall(r'(\w+)=([0-9A-F]+)(?= |$)',line)}
            fields.update(step=step, voice=False)
            fields['instrument']=int(re.search(r'packet=([0-9A-F]{8})',line)[1],16)>>16
            fields['missing']=fields['selector']==9 and fields['instrument'] not in loaded
            stack.append(fields)
        elif line.startswith('AUDIO_VOICE '):
            assert stack and stack[-1]['selector']==9, 'UNLOADED NOTE LIVE / VOICE OWNER'
            stack[-1]['voice']=True
        elif line.startswith('AUDIO_RETURN '):
            fields={k:int(v,16) for k,v in re.findall(r'(\w+)=([0-9A-F]+)',line)}
            assert stack, 'UNLOADED NOTE LIVE / RETURN OWNER'
            call=stack.pop();selector=call['selector']
            assert (selector,call['sp'])==(fields['selector'],fields['sp']), 'UNLOADED NOTE LIVE / RETURN'
            if selector in (34,35,36):
                assert fields['d0']==0, 'UNLOADED NOTE LIVE / LOAD RESULT'
                if selector==34:loaded.add(call['arg'])
                elif selector==36:loaded.discard(call['arg'])
                else:loaded=set(range(34))
            if call['missing']:missing.append((call['step'],call['instrument'],fields['d0'],call['voice']))
    assert not stack and missing==[(1467,32,0,False)], ('UNLOADED NOTE LIVE / REQUEST',missing)
    print('UNLOADED NOTE LIVE PASS: original level-four instrument 32 returns zero without a voice')


if __name__ == '__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('reference',type=Path)
    parser.add_argument('--route',type=Path)
    args=parser.parse_args();check(args.reference)
    if args.route:check_route(args.route)
