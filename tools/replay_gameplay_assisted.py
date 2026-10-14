"""Repeat the pinned assisted entry survey in a fresh opted-in Macintosh run."""
import argparse
from pathlib import Path
from gameplay_assisted_inputs import REFERENCE, MANIFEST, metadata
from gameplay_assist import assist
from replay_gameplay_route import KEYS, compare, status, wait
import time


def replay(run, reference=REFERENCE, manifest=MANIFEST):
    metadata(reference, manifest)
    final = status(reference)
    initial = wait(run, 0, 600)
    assert initial['reason']=='first-frame' and initial['level']=='1', 'ASSIST REPLAY / INITIAL'
    compare(run, reference, 0)
    for line in (reference/'route-actions.txt').read_text().splitlines():
        fields=line.split();sequence=int(fields[0])
        assert sequence==int(status(run)['sequence'])+1, 'ASSIST REPLAY / SEQUENCE'
        if len(fields)==4 and fields[1]=='assist':
            assist(run,fields[2],int(fields[3]))
        else:
            assert fields[1:] == ['finish'] or (len(fields)==3 and (fields[2]=='none' or set(fields[2].split(','))<=KEYS) and 1<=int(fields[1])<=600), 'ASSIST REPLAY / COMMAND'
            temporary=run/'route-command.tmp';temporary.write_text(line+'\n');temporary.replace(run/'route-command.txt')
            wait(run,sequence,120)
        if str(sequence)==final['sequence'] and final['reason']=='ending-return':
            deadline=time.monotonic()+120
            while status(run).get('reason')!='ending-return':
                if time.monotonic()>deadline:raise TimeoutError('ASSIST REPLAY / ENDING RETURN MISSING')
                time.sleep(.2)
        compare(run,reference,sequence)
    print('ASSISTED ORIGINAL REPEAT PASS: all recorded checkpoints; scope is the pinned route')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('run',type=Path)
    parser.add_argument('--reference',type=Path,default=REFERENCE)
    parser.add_argument('--manifest',type=Path,default=MANIFEST)
    args=parser.parse_args();replay(args.run,args.reference,args.manifest)
