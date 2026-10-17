"""Check the original freed-master layout matrix and the supported game aliases."""
import argparse
from collections import Counter
import hashlib
from pathlib import Path
import re
from extract_original_data import ROOT, pins
from check_handle_state_contract import check as game_contract

COUNTS=(1,2,3,8,32,64,65)
FIELDS=('count','reverse','index','trap','slot','master','preceding','d0','error','ccr','unchanged','changed')

def check(text):
    assert 'FAIL REFERENCE' not in text, 'reference failure'
    for marker in ('FREED_FIXTURE_COMPLETE','FREED_FIXTURE_RESTORED'):
        assert text.count(marker+'\n')==1, 'completion/restoration missing'
    lines=[line for line in text.splitlines() if line.startswith('FREED ')]
    expected=[(n,r,i,t) for n in COUNTS for r in (0,1) for i in range(1,n+1) for t in (0xa02a,0xa049)]
    assert len(lines)==len(expected)==700, 'case count'
    rows=[]
    for line,key in zip(lines,expected):
        values=dict(field.split('=') for field in line.split()[1:])
        assert tuple(values)==FIELDS, 'fields'
        row={k:int(v,10 if k in ('count','reverse','index','unchanged','changed') else 16) for k,v in values.items()}
        assert tuple(row[k] for k in FIELDS[:4])==key, 'case order'
        # These outcomes describe the pinned private-zone experiment only.
        # They are deliberately NOT a native invalid-pointer classifier.
        master=row['master']
        error=0xff93 if master==0 else 0xff8f if master==0x40 else 0 if master==0x3c else 0xff91
        assert row['error']==error, 'layout error'
        assert row['d0']==(error|0xffff0000 if error else 0), 'D0'
        assert row['ccr']&15==(8 if error else 4), 'NZVC'
        changed=int(master==0x3c and row['trap']==0xa049)
        assert (row['unchanged'],row['changed'])==(1-changed,changed), 'heap mutation'
        rows.append(row)
    assert Counter(r['error'] for r in rows)=={0xff91:682,0xff8f:8,0xff93:4,0:6}, 'outcomes'
    assert text.count('FREED_FIXTURE_ISOLATION_RESTORE\n')==3, 'mutation isolation'
    live=re.findall(r'^FREED_LIVE count=(\d+) reverse=([01]) trap=([0-9A-F]+) error=0000$',text,re.M)
    assert [(int(n),int(r),int(t,16)) for n,r,t in live]==[(n,r,t) for n in COUNTS for r in (0,1) for t in (0xa02a,0xa049)], 'live controls'
    return rows

def controls(text):
    mutations=[('FREED_FIXTURE_COMPLETE','INCOMPLETE'),('FREED_FIXTURE_RESTORED','NOT_RESTORED'),
               ('error=FF8F','error=FF91'),('d0=FFFFFF91','d0=00000000'),
               ('unchanged=0 changed=1','unchanged=1 changed=0'),
               ('FREED_FIXTURE_ISOLATION_RESTORE','ISOLATION_MISSING'),
               ('FREED_LIVE count=1','FREED_LIVE count=0'),('index=1','index=2')]
    for old,new in mutations:
        altered=text.replace(old,new,1);assert altered!=text
        try:check(altered)
        except AssertionError:pass
        else:raise AssertionError('accepted altered evidence: '+old)
    print('FREED CONTROLS PASS: eight missing/altered outcomes rejected')

def game_check(text, ending=False):
    assert 'FAIL REFERENCE' not in text, 'game reference failed'
    marker='GAMEPLAY_ENDING_COMPLETE' if ending else 'GAMEPLAY_EXPIRED_COMPLETE'
    assert text.count(marker+'\n')==1, 'game completion'
    if ending:
        text=text[text.index('GAMEPLAY_VICTORY'):]
    else:
        assert text.count('GAMEPLAY_EXPIRED_RETURN result=1\n')==1, 'game return'
    rows=re.findall(r'^FREED_GAME (before|after) site=0x([0-9A-F]+) handle=([0-9A-F]+) master=([0-9A-F]+)(?: d0=([0-9A-F]+) error=([0-9A-F]+))?$',text,re.M)
    assert rows and len(rows)%2==0, 'game call pairs'
    failed=set()
    for before,after in zip(rows[::2],rows[1::2]):
        assert (before[0],after[0])==('before','after') and before[1:3]==after[1:3], 'game pairing'
        if after[5]!='0000':
            expected=('FFFFFF8F','FF8F') if ending and after[1]=='63AA' else ('FFFFFF91','FF91')
            assert after[4:]==expected, 'game error'
            assert before[3]==after[3] and int(before[2],16) and int(before[3],16), 'game non-null master preserved'
            failed.add(int(after[1],16))
        else:assert after[4]=='00000000', 'game live result'
    assert failed==({0x63aa,0x29b0,0x29c8,0x29e0} if ending else {0x63aa,0x29c8}), 'game supported error sites'


def memory_check(text):
    game_check(text,ending=True)
    assert text.count('FREED_MEMERROR_ARMED physical-alias-verified\n')==1, 'observer alias'
    reads=re.findall(r'^FREED_MEMERROR read pc=([0-9A-F]+) value=([0-9A-F]+) mask=FFFF0000$',text,re.M)
    assert len(reads)>100 and any(int(pc,16)<0x800000 for pc,_ in reads), 'positive CPU read controls'
    suffix=text[text.index('GAMEPLAY_VICTORY'):]
    before=suffix.index('FREED_GAME before site=0x63AA')
    after=suffix.index('FREED_GAME after site=0x63AA',before)
    assert re.search(r'^FREED_MEMERROR write pc=[0-9A-F]+ value=FF8F mask=FFFF0000$',suffix[before:after],re.M), 'error write control'
    events=re.findall(r'^FREED_MEMERROR (read|write) pc=([0-9A-F]+) value=([0-9A-F]+) mask=([0-9A-F]+)$',suffix[after:],re.M)
    assert events and events[0][0]=='write' and events[0][2]=='0000' and events[0][3]=='FFFF0000', 'error must be cleared before read'
    return suffix[after:].splitlines()[0]


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('run',type=Path);p.add_argument('--controls',action='store_true');a=p.parse_args()
    path=a.run/'session.log'
    manifest=pins(ROOT/'manifest/freed-handles-evidence.sha256')
    key=str(path.resolve().relative_to(ROOT))
    assert key in manifest and hashlib.sha256(path.read_bytes()).hexdigest()==manifest[key], 'original pin'
    text=path.read_text();check(text)
    game_contract(ROOT/'ref/mame/runs/gameplay-hunlock-contract-1')
    game_contract(ROOT/'ref/mame/runs/gameplay-assisted-handles-1',live=True)
    game_contract(ROOT/'ref/mame/runs/gameplay-assisted-ending-handles-1',ending=True)
    game=ROOT/'ref/mame/runs/freed-game-3/session.log'
    assert hashlib.sha256(game.read_bytes()).hexdigest()==manifest[str(game.relative_to(ROOT))], 'game pin'
    game_text=game.read_text();game_check(game_text)
    ending=ROOT/'ref/mame/runs/freed-ending-1/session.log'
    assert hashlib.sha256(ending.read_bytes()).hexdigest()==manifest[str(ending.relative_to(ROOT))], 'ending pin'
    game_check(ending.read_text(),ending=True)
    memory=ROOT/'ref/mame/runs/freed-ending-memory-2/session.log'
    assert hashlib.sha256(memory.read_bytes()).hexdigest()==manifest[str(memory.relative_to(ROOT))], 'memory observer pin'
    memory_text=memory.read_text();anchor=memory_check(memory_text)
    if a.controls:
        for changed in (memory_text.replace(anchor,anchor+'\nFREED_MEMERROR read pc=00410000 value=FF00 mask=FF000000',1),
                        memory_text.replace('FREED_MEMERROR_ARMED','MISSING_OBSERVER',1),
                        memory_text.replace('FREED_MEMERROR read','MISSING_READ'),
                        memory_text.replace('value=FF8F mask=FFFF0000','value=0000 mask=FFFF0000')):
            assert changed!=memory_text
            try:memory_check(changed)
            except AssertionError:pass
            else:raise AssertionError('accepted altered error lifetime evidence')
        print('FREED ERROR LIFETIME CONTROLS PASS: four missing/altered observations rejected')
        controls(text)
        for old,new in (('error=FF91','error=FF8F'),('GAMEPLAY_EXPIRED_COMPLETE','INCOMPLETE'),('site=0x63AA','site=0x63AC')):
            changed=game_text.replace(old,new,1);assert changed!=game_text
            try:game_check(changed)
            except AssertionError:pass
            else:raise AssertionError('accepted altered game call')
        print('FREED GAME CONTROLS PASS: three altered live-call records rejected')
    print('FREED ORIGINAL PASS: 700 layout cases, 28 live controls, three explicit mutation restorations; supported aliases retain their separate paired contracts')

if __name__=='__main__':main()
