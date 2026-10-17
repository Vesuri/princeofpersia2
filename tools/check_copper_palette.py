"""Decode every observed inactive copper palette against the Macintosh table."""
import argparse,re
from extract_original_data import ROOT,pins
import hashlib
from pathlib import Path
from aprof_report import check_run
from check_display_scenes import decode_copper


def pair(copper,table,transfer,base,parity):
    assert len(table)==2056 and len(transfer)==256,'COPPER / INPUT EXTENT'
    rgb=bytes(transfer[table[10+i*8+c]] for i in range(256) for c in (0,2,4))
    assert decode_copper(copper,base,parity)==rgb,'COPPER / PALETTE'
    return rgb


def check(run):
    check_run(run)
    log=(run/'gdb-out.log').read_text()
    rows=re.findall(r'^COPPER_FRAME (\d+) base=([0-9a-f]+) crop=(\d+),(\d+)$',log,re.M)
    # The final opening record occurs after the final observed publication.
    count=int(re.search(r'^OPENING RECORDED rows=(\d+) phases=4$',log,re.M)[1])
    assert len(rows)==count and [int(r[0]) for r in rows]==list(range(1,count+1)),'COPPER / PUBLICATION COUNT'
    previous=None;changed=stable=0
    for frame,base,top,left in rows:
        table=(run/f'copper-{frame}-colors.bin').read_bytes()
        transfer=(run/f'copper-{frame}-transfer.bin').read_bytes()
        assert len(table)==2056 and len(transfer)==256,'COPPER / INPUT EXTENT'
        rgb=bytes(transfer[table[10+i*8+c]] for i in range(256) for c in (0,2,4))
        for parity,name in enumerate(('even','odd')):
            copper=(run/f'copper-{frame}-{name}.bin').read_bytes()
            assert pair(copper,table,transfer,int(base,16),parity)==rgb
        if previous==rgb:stable+=1
        elif previous is not None:changed+=1
        previous=rgb
    assert stable,'COPPER / UNCHANGED PALETTE NOT EXERCISED'
    # Make sure comparisons reject pixels as well as extents and pointers.
    rejected=0
    def reject(c,t=table,v=transfer,b=int(base,16),p=1):
        nonlocal rejected
        try:pair(c,t,v,b,p)
        except (AssertionError,KeyError):rejected+=1
        else:raise AssertionError('COPPER / INACTIVE REJECTION CONTROL')
    for offset in (2,4,123,127,len(copper)-1):
        bad=bytearray(copper);bad[offset]^=1;reject(bad)
    reject(copper[:-1]);reject(copper,t=table[:-1]);reject(copper,v=transfer[:-1]);reject(copper,b=int(base,16)+4);reject(copper,p=0)
    bad=bytearray(transfer);bad[table[10]]^=1;reject(copper,v=bad)
    print('COPPER CONTROLS PASS:',rejected,'rejected alterations')
    print('COPPER PALETTE PASS:',len(rows),'publications, both fields;',stable,'unchanged,',changed,'changed transitions; original colour table and transfer')


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('run',type=Path);a=p.parse_args()
    recorded=pins(ROOT/'manifest/performance-68020-palette.sha256')
    paths=[a.run/'gdb-out.log',*a.run.glob('copper-*.bin')]
    for path in paths:
        key=str(path.resolve().relative_to(ROOT))
        assert key in recorded and hashlib.sha256(path.read_bytes()).hexdigest()==recorded[key],('COPPER / EVIDENCE PIN',key)
    check(a.run)
