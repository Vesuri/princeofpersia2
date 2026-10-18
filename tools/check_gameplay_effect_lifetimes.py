"""Pair original gameplay effect completion blocks with the production audio engine."""
import argparse
import hashlib
import re
import subprocess
from pathlib import Path
from extract_original_data import ROOT, pins


def events(log):
    driver = re.findall(r'^AUDIO_INSTALLED base=([0-9A-F]+) size=30960$', log, re.M)
    if len(driver) != 1 or 'GAMEPLAY_ROUTE_COMPLETE level=1 next=2\n' not in log:
        raise ValueError('EFFECT LIFETIME / ORIGINAL COMPLETION')
    driver = int(driver[0], 16)
    active = False; pending = None; result = []
    for line in log.splitlines():
        if line.startswith('GAMEPLAY_ROUTE_CHECKPOINT sequence=0 '):
            active = True
        if not active:
            continue
        if line.startswith('AUDIO_CALL selector=11 '):
            fields = dict(re.findall(r'(\w+)=([0-9A-F/]+)', line))
            packet = fields['packet'].split('/')
            pending = (int(packet[1], 16), int(packet[2], 16), int(fields['clock'], 16))
        elif line.startswith('AUDIO_COMPLETE slot=5 ') and pending:
            fields = dict(re.findall(r'(\w+)=([0-9A-F]+)', line))
            # DERIVED original mixer return; replacement/explicit stop callbacks
            # use different callers and are not natural sample completion.
            if (int(fields['caller'], 16) & 0xffffff) - driver == 0x2c7c:
                size, rate, start = pending
                elapsed = int(fields['clock'], 16) - start
                if not 0 < elapsed < 32768:
                    raise ValueError('EFFECT LIFETIME / CLOCK ORDER')
                result.append((size, rate, elapsed)); pending = None
    if len(result) != 144:
        raise ValueError('EFFECT LIFETIME / POSITIVE SAMPLE COUNT')
    return result


def check(reference):
    recorded = pins(ROOT/'manifest/gameplay-effect-lifetimes.sha256')
    if str((reference/'session.log').resolve().relative_to(ROOT)) not in recorded:
        raise ValueError('EFFECT LIFETIME / UNPINNED REFERENCE')
    for name, digest in recorded.items():
        if hashlib.sha256((ROOT/name).read_bytes()).hexdigest() != digest:
            raise ValueError('EFFECT LIFETIME / INPUT PIN '+name)
    log = (reference/'session.log').read_text()
    rows = events(log)
    output = ROOT/'tmp/item16-effects'; output.mkdir(exist_ok=True)
    exe = output/'probe'
    subprocess.run(['c++', '-std=c++17', '-fsanitize=address,undefined', '-Isrc',
                    'tools/probe_effect_lifetime.cpp', '-o', str(exe)], cwd=ROOT, check=True)
    data = ''.join(f'{size} {rate}\n' for size, rate, _ in rows)
    native = subprocess.run([str(exe)], input=data, text=True, capture_output=True, check=True)
    actual = [int(v) for v in native.stdout.splitlines()]
    if actual != [r[2] for r in rows]:
        raise ValueError('EFFECT LIFETIME / ORIGINAL PAIR')
    # Missing route completion and missing natural-completion records reject.
    for changed in (log.replace('GAMEPLAY_ROUTE_COMPLETE level=1 next=2', 'INCOMPLETE'),
                    log.replace('AUDIO_COMPLETE slot=5 ', 'MISSING slot=5 ')):
        try:
            events(changed)
        except ValueError:
            continue
        raise ValueError('EFFECT LIFETIME / CONTROL ACCEPTED')
    print('EFFECT LIFETIME PASS: 144 original natural completions match the production '
          'engine; ASan/UBSan; two missing-evidence controls. No wall-clock claim.')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('reference', type=Path)
    check(parser.parse_args().reference)
