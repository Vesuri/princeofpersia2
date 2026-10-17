"""Interpret observed dissolve clocks using the original feedback rule.

This checks existing records; it does not simulate replacement game logic.
Quantum targets are progress ticks, not displayed frame deadlines.
"""
import hashlib
import argparse
import json
import re
from pathlib import Path

from aprof_scene_report import read
from aprof_report import require
from extract_original_data import ROOT, pins


def feedback(quanta):
    require(bool(quanta), 'DISSOLVE EMPTY FEEDBACK')
    require([q['progress'] for q in quanta] == list(range(1, len(quanta)+1)),
            'DISSOLVE PROGRESS')
    for previous, current in zip(quanta, quanta[1:]):
        lag, delay = previous['late_ticks'], previous['delay']
        # CODE 15+$3222..$3248: one increment while ahead, one decrement
        # while behind unless already zero, no change when on target.
        expected = delay + (1 if lag < 0 else -1 if lag > 0 and delay else 0)
        require(current['delay'] == expected, 'DISSOLVE FEEDBACK TRANSITION')
    return dict(quanta=len(quanta), initial_delay=quanta[0]['delay'],
                delay_range=[min(q['delay'] for q in quanta), max(q['delay'] for q in quanta)],
                signed_lag_range=[min(q['late_ticks'] for q in quanta), max(q['late_ticks'] for q in quanta)],
                final_lag_ticks=quanta[-1]['late_ticks'],
                late_quanta=sum(q['late_ticks'] > 0 for q in quanta),
                feedback_transitions_checked=len(quanta)-1)


def original(path):
    log=(path/'session.log').read_text()
    require(log.count('GAMEPLAY_DISCOVERY_COMPLETE\n')==1 and 'FAIL REFERENCE' not in log,
            'DISSOLVE ORIGINAL COMPLETION')
    returns=re.findall(r'^CUTSCENE_RETURN ordinal=(\d+) selector=6 result=0 copy=\d+ text=\d+$',log,re.M)
    require(len(returns)==1, 'DISSOLVE ORIGINAL RETURN')
    rows=re.findall(r'^SCENE_CLOCK ordinal='+returns[0]+r' selector=6 site=([0-9A-F]+) ticks=(\d+) duration=(\d+) progress=(\d+) target=(\d+) delay=(\d+)$',log,re.M)
    draws=[]
    for site,tick,duration,progress,target,delay in rows:
        tick,duration,progress,target,delay=map(int,(tick,duration,progress,target,delay))
        if site=='3152':draws.append(dict(duration=duration,start_tick=tick,quanta=[]))
        elif site=='321A':
            require(draws and duration==draws[-1]['duration'] and target==draws[-1]['start_tick']+progress,
                    'DISSOLVE ORIGINAL TARGET')
            draws[-1]['quanta'].append(dict(progress=progress,late_ticks=tick-target,delay=delay))
    require([d['duration'] for d in draws]==[240,180,20,20,20,180], 'DISSOLVE ORIGINAL COVERAGE')
    return draws


def verify_inputs(paths, manifest):
    recorded=pins(ROOT/'manifest'/manifest)
    for path in paths:
        name=str(path.resolve().relative_to(ROOT))
        require(name in recorded and hashlib.sha256(path.read_bytes()).hexdigest()==recorded[name],
                'DISSOLVE INPUT PIN '+name)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('native')
    parser.add_argument('reference',type=Path)
    args=parser.parse_args()
    directory=ROOT/'tmp/aprof'/args.native
    verify_inputs([directory/name for name in ('config.txt','gdb-out.log','scene-timers.bin',
        'scene-clocks.bin','scene-start-a5.bin','scene-end-a5.bin','build-receipt.json')], 'scenes-profile.sha256')
    verify_inputs([args.reference/'session.log'], 'scenes-reference.sha256')
    native=read(args.native)['draws'];reference=original(args.reference)
    require([(d['duration'],len(d['quanta'])) for d in native]==
            [(d['duration'],len(d['quanta'])) for d in reference], 'DISSOLVE MATCHED WORKLOAD')
    print(json.dumps(dict(native=args.native,reference=args.reference.name,
        draws=[dict(duration=n['duration'],native=feedback(n['quanta']),original=feedback(o['quanta']))
               for n,o in zip(native,reference)]),indent=2))


if __name__=='__main__':main()
