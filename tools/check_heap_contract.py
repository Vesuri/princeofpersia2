"""Original System 7.5.5 placement/resize cases paired by test_mac_heap.cpp."""
import argparse
import re
from pathlib import Path

NAMES = ('free-above','locked-barrier','no-room','purgeable','fragmented',
         'interior-hole','small-gap','small-tail','purgeable-no-room')


def check(text):
    assert 'FAIL REFERENCE' not in text
    assert text.count('HEAP_FIXTURE_COMPLETE movehhi=9 resize=5\n')==1
    locked=re.findall(r'^MOVEHHI_LOCKED state=([0-9A-F]+) d0=([0-9A-F]+) error=([0-9A-F]+) ccr=([0-9A-F]+) unchanged=(\d+)$',text,re.M)
    assert locked==[(state,'FFFFFF8B','FF8B','8','1') for state in ('80','C0','A0','E0')]
    rows=re.findall(r'^MOVEHHI (\S+) error=(-?\d+) result=([0-9A-F]+) target=([0-9A-F]+)->([0-9A-F]+) free=(\d+)->(\d+) other1=([0-9A-F]+)->([0-9A-F]+)$',text,re.M)
    assert tuple(r[0] for r in rows)==NAMES
    for name,error,result,start,end,free,after,other,moved in rows:
        assert int(error)==0 and int(result,16)==0 and free==after
        start,end,other,moved=map(lambda v:int(v,16),(start,end,other,moved))
        if name=='locked-barrier':assert start==end and other==moved
        elif name in ('free-above','purgeable'):assert end>start and other==moved
        else:assert end>start and moved==start
    assert re.findall(r'^HEAP_RESIZE size=(\d+) preserved=256 moved=0 error=0$',text,re.M)==['1024','256','512','1024']
    assert text.count('HEAP_RESIZE failure=-108 size=1024 preserved=256 moved=0\n')==1


def controls(text):
    for old,new in [('HEAP_FIXTURE_COMPLETE','INCOMPLETE'),('MOVEHHI free-above','MOVEHHI invalid'),
                    ('error=0 result=0','error=1 result=0'),('failure=-108','failure=0'),
                    ('d0=FFFFFF8B','d0=00000000'),('error=FF8B','error=0000'),
                    ('ccr=8 unchanged=1','ccr=4 unchanged=1'),('unchanged=1','unchanged=0'),
                    ('size=256 preserved=256','size=255 preserved=256')]:
        changed=text.replace(old,new,1);assert changed!=text
        try:check(changed)
        except AssertionError:pass
        else:raise AssertionError('accepted altered heap evidence')
    print('HEAP ORIGINAL CONTROLS PASS: nine altered records rejected')


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('reference',type=Path)
    p.add_argument('--controls',action='store_true');a=p.parse_args()
    text=(a.reference/'session.log').read_text();check(text)
    if a.controls:controls(text)
    print('HEAP ORIGINAL PASS: nine MoveHHi placements; same-size, shrink, growth and locked low-memory failure')
