"""Check actual opening draw coverage and full planes, separately from timing."""
import argparse
import re
from pathlib import Path
from check_display_scenes import decode_planes
from aprof_report import check_run

def rows(text):
    result=[];phase=0
    for line in text.splitlines():
        m=re.fullmatch(r'BOUNDS frame=(\d+) crop=(\d+),(\d+) dirty=(\d+) converted=(\d+)',line)
        if m:
            frame,top,left,nd,nc=map(int,m.groups())
            result.append(dict(frame=frame,phase=phase,top=top,left=left,nd=nd,nc=nc,dirty=[],converted=[]))
        elif line.startswith('OPENING PHASE '):
            assert int(line.split()[2])==phase,'phase order'
            phase+=1
        elif line.startswith(('DIRTY ','CONVERTED ')):
            kind,*values=line.split();assert result,'bounds header missing'
            result[-1][kind.lower()].append(tuple(map(int,values)))
    return result

def coverage(row,minimal=True):
    assert len(row['dirty'])==row['nd'] and len(row['converted'])==row['nc'],'rectangle count'
    area=minimum=0
    for t,l,b,r in row['converted']:
        assert 0<=t<b<=384 and 0<=l<r<=512 and l%16==0 and (r-l)%32==0,'illegal conversion'
    for y in range(384):
        needed=[False]*32;covered=[0]*32
        for t,l,b,r in row['dirty']:
            assert t<=b and l<=r,'inverted dirty'
            if t<=y+row['top']<b:
                for x in range(max(0,l-row['left']),min(512,r-row['left'])):needed[x//16]=True
        for t,l,b,r in row['converted']:
            if t<=y<b:
                for x in range(l//16,r//16):covered[x]+=1
        assert all(not n or covered[x] for x,n in enumerate(needed)),'missing dirty coverage'
        best=[100]*33;best[32]=0
        for x in range(31,-1,-1):
            if not needed[x]:best[x]=best[x+1]
            for end in range(x+2,33,2):best[x]=min(best[x],end-x+best[end])
        if minimal:
            assert max(covered)<=1,'overlap'
            assert sum(covered)==best[0],('nonminimal',row['frame'],y)
        area+=sum(covered)*16;minimum+=best[0]*16
    return area,minimum

def check(run,minimal=True,phase_counts=(7,28,9,9,28)):
    assert len(phase_counts)==5 and all(n>0 for n in phase_counts),"publication contract"
    total=sum(phase_counts)
    check_run(run)
    records=rows((run/'gdb-out.log').read_text())
    assert len(records)==total and [r['frame'] for r in records]==list(range(1,total+1)),'consecutive publications'
    # The boundary fires after landing publication, before its row is stored.
    # Thus the existing six-row phase-0 profile contains seven publications.
    assert [sum(r['phase']==p for r in records) for p in range(5)]==list(phase_counts),'publication phase boundaries'
    assert any(r['nd']>1 for r in records if r['phase']==0),'combined landing rectangles unexercised'
    areas=[]
    for r in records:
        areas.append(coverage(r,minimal))
        chunky=(run/f"bounds-{r['frame']}-chunky.bin").read_bytes();assert len(chunky)==307200
        expected=b''.join(chunky[(y+r['top'])*640+r['left']:(y+r['top'])*640+r['left']+512] for y in range(384))
        assert decode_planes((run/f"bounds-{r['frame']}-planes.bin").read_bytes())==expected,('plane mismatch',r['frame'])
    print('PASS C2P bounds:',total,'full plane comparisons; landing converted/minimum pixels',[area for r,area in zip(records,areas) if r['phase']==0])
    return records,areas
def compare(run,reference,phase_counts=(7,28,9,9,28)):
    check_run(reference)
    a=rows((run/'gdb-out.log').read_text());b=rows((reference/'gdb-out.log').read_text())
    assert len(a)==len(b)==sum(phase_counts),'comparison extent'
    for index,(x,y) in enumerate(zip(a,b),1):
        assert x['frame']==y['frame']==index and (x['top'],x['left'])==(y['top'],y['left']),'comparison viewport'
        first=(run/f'bounds-{index}-chunky.bin').read_bytes()
        second=(reference/f'bounds-{index}-chunky.bin').read_bytes()
        assert len(first)==len(second)==307200,'comparison image extent'
        for row in range(x['top'],x['top']+384):
            start=row*640+x['left']
            assert first[start:start+512]==second[start:start+512],('changed viewport',index,row)
    print('PASS native before/after:',len(a),'complete viewports are byte-identical')

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('run',type=Path);p.add_argument('--baseline',action='store_true')
    p.add_argument('--compare',type=Path)
    p.add_argument('--phase-counts',type=int,nargs=5,default=(7,28,9,9,28),help='explicit publication workload; does not qualify cadence')
    a=p.parse_args();check(a.run,not a.baseline,a.phase_counts)
    if a.compare:compare(a.run,a.compare,a.phase_counts)
