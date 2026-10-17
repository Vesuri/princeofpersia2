"""Observe real gameplay wrapper entries and pair save/load return values/CCR."""
import argparse
import json
from pathlib import Path
import re
import tempfile
from build_wrapper_fixture import WRAPPERS
from extract_original_data import ROOT

GROUPS={'restart':(0,1,2,8,9),'save':(0,1,2,6,7,8,9),
        'hall':(0,1,2,3,4,5),'route':(0,1,2,4,5)}


def generate(group):
    # This prefix precedes every breakpoint in the existing fixture script.
    # One observation per wrapper bounds debugger overhead.
    lines=[]
    for index,which in enumerate(GROUPS[group],1):
        name=WRAPPERS[which]
        lines.extend((f'break *((unsigned long)&{name}+10)','commands',' silent',
                      ' if *(unsigned long*)($pc-10)!=0x4feffffc || *(unsigned long*)($pc-6)!=0x48e7fffe || *(unsigned short*)($pc-2)!=0x42e7',
                      '  echo WRAPPER / LIVE PROLOGUE MISMATCH\\n','  detach','  quit 1',' end',
                      f' printf "WRAPPER SAVED {name} d0=%08lx ccr=%02lx debug_ccr=%02lx\\n",$d0,(*(unsigned short*)$sp & 31),((unsigned int)$sr & 31)',
                      f' disable {index}',' continue','end'))
    return '\n'.join(lines)+'\n'+(ROOT/f'amiga/gameplay_{group}.gdb').read_text()


def pair(original,native):
    source=(original/'session.log').read_text()
    target=(native/'gdb-out.log').read_text()
    assert 'GAMEPLAY_REFERENCE_COMPLETE route=save-load loaded_frames=1\n' in source
    assert 'FAIL REFERENCE' not in source
    assert 'CORE REASON GAMEPLAY / SAVE LOAD COMPLETE\n' in target
    for label,name in [('save-return','pop2_gameplay_saved'),('load-return','pop2_gameplay_loaded')]:
        left=re.findall(r'^WRAPPER_LIVE label='+label+r' d0=([0-9A-F]{8}) ccr=([0-9A-F]{2})$',source,re.M)
        right=re.findall(r'^WRAPPER SAVED '+name+r' d0=([0-9a-f]{8}) ccr=([0-9a-f]{2}) debug_ccr=[0-9a-f]{2}$',target,re.M)
        assert len(left)==len(right)==1,('WRAPPER LIVE / REACHED',label)
        assert tuple(int(x,16) for x in left[0])==tuple(int(x,16) for x in right[0]),('WRAPPER LIVE / ORIGINAL PAIR',label,left,right)
    print('WRAPPER LIVE PASS: original RTS and native CPU-saved CCR agree for save/load D0 and CCR')


def coverage(runs,integration):
    reached=set()
    for run in runs:
        log=(run/'gdb-out.log').read_text()
        reached.update(re.findall(r'^WRAPPER (?:LIVE|SAVED) (\w+) d0=',log,re.M))
    # Normal integration's kind=3 is set by the death-entry callback, and
    # stage 4 by its return callback. Save/Hall builds have other stage-4 paths.
    log=(integration/'gdb-out.log').read_text()
    flags=json.loads((integration/'build-receipt.json').read_text())['flags'].split()
    assert set(flags)=={'-DPOP2_CORE','-DPOP2_DISPLAY','-DPOP2_INPUT','-DPOP2_UI',
                        '-DPOP2_AUDIO','-DPOP2_INTEGRATION_PROBE','-DPOP2_VERIFY'}
    assert re.search(r'^INTEGRATION STAGE 4 fields=\d+ right=\d+ kind=3$',log,re.M)
    assert 'CORE REASON INTEGRATION / ROUTE COMPLETE\n' in log
    assert 'INTEGRATION RESTORED\n' in log
    reached.update(WRAPPERS[8:])
    assert reached==set(WRAPPERS),('WRAPPER LIVE / COVERAGE',sorted(set(WRAPPERS)-reached))
    print('WRAPPER LIVE COVERAGE PASS: all ten wrappers reached; route correctness is checked by the existing gameplay gates')


def controls(original,native):
    good=(native/'gdb-out.log').read_text()
    prefix='WRAPPER SAVED pop2_gameplay_saved d0=00000001 ccr=00'
    assert prefix in good
    changes=[(prefix,prefix.replace('00000001','00000000')),
             (prefix,prefix.replace('ccr=00','ccr=04')),
             (prefix,'MISSING WRAPPER SAMPLE'),
             ('CORE REASON GAMEPLAY / SAVE LOAD COMPLETE','INCOMPLETE')]
    with tempfile.TemporaryDirectory(prefix='wrapper-live-controls-',dir=ROOT/'tmp') as directory:
        scratch=Path(directory)
        for before,after in changes:
            assert before in good
            (scratch/'gdb-out.log').write_text(good.replace(before,after))
            try:pair(original,scratch)
            except AssertionError:pass
            else:raise AssertionError('WRAPPER LIVE / INACTIVE REJECTION CONTROL')
    print('WRAPPER LIVE CONTROLS PASS: 4 rejected alterations, including the observed incorrect CCR 4')


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    sub=p.add_subparsers(dest='mode',required=True)
    g=sub.add_parser('generate');g.add_argument('group',choices=GROUPS);g.add_argument('output',type=Path)
    c=sub.add_parser('check');c.add_argument('original',type=Path);c.add_argument('native',type=Path)
    c.add_argument('--controls',action='store_true')
    c=sub.add_parser('coverage');c.add_argument('runs',type=Path,nargs='+')
    c.add_argument('--integration',type=Path,required=True)
    a=p.parse_args()
    if a.mode=='generate':a.output.write_text(generate(a.group))
    elif a.mode=='check':
        pair(a.original,a.native)
        if a.controls:controls(a.original,a.native)
    else:coverage(a.runs,a.integration)
