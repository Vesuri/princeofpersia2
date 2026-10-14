"""Validate the original valid/disposed HUnlock contract and preserve evidence."""
import argparse
from pathlib import Path
import re
import hashlib
from extract_original_data import ROOT, pins


def check(run, live=False, ending=False):
    manifest=ROOT/'manifest'/('gameplay-ending-handles.sha256' if ending else 'gameplay-hunlock-live.sha256' if live else 'gameplay-hunlock-contract.sha256')
    for name,expected in pins(manifest).items():
        assert hashlib.sha256((run/name).read_bytes()).hexdigest()==expected, 'HANDLE CONTRACT / PIN'
    text=(run/'session.log').read_text()
    if ending:
        start=text.index('GAMEPLAY_VICTORY')
        rows=re.findall(r'^GAMEPLAY_HANDLE phase=handle-(before|after) offset=29[AD][AC] a0=([0-9A-F]+) master=([0-9A-F]+) memerr=([0-9A-F]+) owners=.*$',text[start:],re.M)
        assert len(rows)==4 and [r[0] for r in rows]==['before','after']*2, 'ENDING HANDLE / CALL PAIRS'
        assert all(rows[i][1:3]==rows[i+1][1:3] and int(rows[i][1],16) for i in (0,2)), 'ENDING HANDLE / PRESERVED MASTERS'
        assert all(rows[i][3]=='FF91' for i in (1,3)), 'ENDING HANDLE / MEMWZERR'
        assert text.count('GAMEPLAY_ENDING_COMPLETE\n')==1, 'ENDING HANDLE / COMPLETION'
        print('ENDING HANDLE PASS: disposed palette alias, memWZErr, unchanged slot, ending returned')
        return
    if live:
        start=text.index('GAMEPLAY_ROUTE_CHECKPOINT sequence=17 frames=120')
        end=text.index('GAMEPLAY_ROUTE_CHECKPOINT sequence=18',start)
        rows=re.findall(r'^GAMEPLAY_HANDLE phase=handle-(before|after) offset=29C[24] a0=([0-9A-F]+) master=([0-9A-F]+) memerr=([0-9A-F]+) owners=.*$',text[start:end],re.M)
        assert len(rows)==2 and rows[0][0]=='before' and rows[1][0]=='after', 'HANDLE LIVE / CALL PAIR'
        assert rows[0][1:3]==rows[1][1:3] and int(rows[0][1],16), 'HANDLE LIVE / PRESERVED MASTER'
        assert rows[1][3]=='FF91', 'HANDLE LIVE / MEMWZERR'
        assert 'GAMEPLAY_ROUTE_NIS_RETURN selector=20' in text[start:end], 'HANDLE LIVE / SCENE RETURN'
        print('HANDLE LIVE CONTRACT PASS: scene 20 released palette alias, memWZErr, unchanged slot, natural return')
        return
    assert text.count('HANDLE_STATE_FIXTURE_COMPLETE\n')==1 and 'FAIL REFERENCE' not in text, 'HANDLE CONTRACT / COMPLETION'
    rows=re.findall(r'^HANDLE_STATE_FIXTURE stage=(\d+) d0=([0-9A-F]+) memerr=([0-9A-F]+) a0=([0-9A-F]+) master=([0-9A-F]+) sr=([0-9A-F]+)$',text,re.M)
    assert len(rows)==3, 'HANDLE CONTRACT / COUNT'
    values=[tuple(int(v,10 if i==0 else 16) for i,v in enumerate(row)) for row in rows]
    assert [r[0] for r in values]==[1,2,3], 'HANDLE CONTRACT / ORDER'
    assert values[0][1:3]==(0,0), 'HANDLE CONTRACT / LIVE CONTROL'
    assert values[1][1:3]==values[2][1:3]==(0xffffff8f,0xff8f), 'HANDLE CONTRACT / DISPOSED ERROR'
    assert len({r[3] for r in values})==1 and values[0][3]!=0, 'HANDLE CONTRACT / SAME MASTER'
    assert values[1][4]==values[2][4], 'HANDLE CONTRACT / FREE LIST PRESERVED'
    assert all(r[5]&15==8 for r in values[1:]), 'HANDLE CONTRACT / CCR'
    print('HANDLE STATE ORIGINAL CONTRACT PASS: valid unlock and disposed unlock/purge')


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('run',type=Path);p.add_argument('--live',action='store_true');p.add_argument('--ending',action='store_true')
    args=p.parse_args();check(args.run,args.live,args.ending)
