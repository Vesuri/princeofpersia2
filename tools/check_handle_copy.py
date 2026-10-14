"""Verify the original HandToHand positive contract and its pinned fixture."""
import argparse
import hashlib
import re
from pathlib import Path
from extract_original_data import ROOT,pins


def check(run):
    for name,digest in pins(ROOT/'manifest/gameplay-handle-copy.sha256').items():
        assert hashlib.sha256((run/name).read_bytes()).hexdigest()==digest,'HANDLE COPY / HASH'
    text=(run/'session.log').read_text()
    assert text.count('HANDLE_COPY_FIXTURE_COMPLETE\n')==1 and 'FAIL REFERENCE' not in text,'HANDLE COPY / COMPLETION'
    rows=re.findall(r'^HANDLE_COPY_FIXTURE stage=(\d) d0=([0-9A-F]+) memerr=([0-9A-F]+) a0=([0-9A-F]+) master=([0-9A-F]+) sr=([0-9A-F]+)$',text,re.M)
    assert len(rows)==4,'HANDLE COPY / COUNT'
    rows=[tuple(int(v,16) for v in row) for row in rows]
    assert [r[0] for r in rows]==[1,2,3,4],'HANDLE COPY / ORDER'
    before,after=rows[1:3]
    assert before[3]!=after[3] and before[4]&0xffffff!=after[4]&0xffffff,'HANDLE COPY / DISTINCT STORAGE'
    assert before[4]>>24==0xc0 and after[4]>>24==0,'HANDLE COPY / STATE'
    assert after[1:3]==(0,0) and after[5]&31==4,'HANDLE COPY / RETURN'
    assert rows[3][1:3]==(16,0),'HANDLE COPY / SIZE'
    assert f'HANDLE_COPY_SOURCE handle={before[3]:08X} master={before[4]:08X}\n' in text,'HANDLE COPY / SOURCE PRESERVED'
    print('ORIGINAL HANDTOHAND PASS: copied bytes/size, distinct ownership, source state, return flags and MemError')


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('run',type=Path)
    check(p.parse_args().run)
