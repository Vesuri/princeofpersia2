"""Pair CPU-executed Jnth calls and independently calculated Paula descriptors."""
import argparse,json,re,struct,math
from pathlib import Path
from audio_replay import calls,ROOT
from extract_original_data import DESTINATION
from check_core_result import restored
import trap_census as census

def fnv(data):
    value=2166136261
    for b in data:value=((value^b)*16777619)&0xffffffff
    return value

def descriptor(row,offset=0):
    if row['selector']==17:
        pcm=row['pcm'];rate=int.from_bytes(row['packet'][8:12],'big')
        period=((3546895<<16)+rate//2)//rate
        converted=bytes(v^128 for v in pcm)+bytes(len(pcm)&1)+bytes(2)
        attack=(len(pcm)+1)&~1
        return (period,64,attack,attack,2,len(converted),fnv(converted))
    inst,note,duration,velocity,channel=struct.unpack('>5H',row['packet'][:10])
    instrument=census.R[b'INST',inst].body
    sample=census.R[b'snd ',int.from_bytes(instrument[:2],'big')].body
    pcm=sample[42:];rate=int.from_bytes(sample[28:32],'big');start,end=struct.unpack('>II',sample[32:40])
    looping=not instrument[5]&32 and start and end and (end-start)&65535>=100
    if not looping:start=end=0
    if offset:
        assert offset<len(pcm),"AUDIO / TAIL SOURCE EXTENT"
        pcm=pcm[offset:];start=end=0
    stride=1;denominator=rate*row['step']
    while ((3546895<<32)*stride+denominator//2)//denominator<124:stride*=2
    period=((3546895<<32)*stride+denominator//2)//denominator
    attack=((math.ceil((end or len(pcm))/stride)+1)//2)*2
    cycle=(end-start)//math.gcd(end-start,stride) if end else 2
    reload=cycle*(1+(cycle&1));converted=bytearray()
    for i in range(attack+reload):
        pos=i*stride
        if not end and i>=attack:converted.append(0);continue
        if end and pos>=end:pos=start+(pos-end)%(end-start)
        converted.append(pcm[pos]^128 if pos<len(pcm) else 0)
    volume=velocity if instrument[5]&64 else 0
    if volume==0 or volume>=125:volume=127
    return (period,(volume*64+63)//127,attack,attack,reload,len(converted),fnv(converted))

def check(run,setup="a1200-fast"):
    metadata=json.loads((run/'dh1/audio-replay.json').read_text())
    original=calls(ROOT/metadata['reference']);log=(run/'gdb-out.log').read_text();restored(run,log,setup)
    assert 'CORE REASON AUDIO / FIXTURE COMPLETE\n' in log,'AUDIO / POSITIVE NATIVE COMPLETION'
    assert 'CORE EXCEPTION vector=0 pc=00000000\n' in log,'AUDIO / CPU EXCEPTION'
    mark=re.search(r'AUDIO REPLAY calls=(\d+) done=(\d+) callbacks=(\d+) abi=(\d+) dma=(\d+)',log)
    assert mark and int(mark[1])==int(mark[2])==int(mark[4])==len(original),'AUDIO / COMPLETION COUNTERS'
    adk=re.search(r'AUDIO ADK saved=(\d+) actual=(\d+)',log)
    assert adk and adk[1]==adk[2],'AUDIO / ADK RESTORE'
    assert 'sound_output=exact' in (run/'logs/debug.uae').read_text(),'AUDIO / PAULA EMULATION DISABLED'
    observed=list(struct.iter_unpack('>6I',(run/'audio-events.bin').read_bytes()))
    assert len(observed)==len(original),'AUDIO / CALL COVERAGE'
    for i,(actual,expected) in enumerate(zip(observed,original)):
        assert actual[0]==expected['result'],('AUDIO / RETURN',i,actual[0],expected['result'])
        if expected['selector']==9:
            assert actual[1:3]==(expected['selected']&0xffffffff,expected['step']),('AUDIO / NOTE',i,actual,expected)
        assert actual[5]==i+1,('AUDIO / ABI',i)
    census.load(DESTINATION/'Prince of Persia 2.rsrc')
    dma=list(struct.iter_unpack('>14I',(run/'audio-dma.bin').read_bytes()))
    all_dma=dma;dma=[r for r in all_dma if not r[12]]
    expected_dma=[(i,r) for i,r in enumerate(original) if r['selector']==17 or r['selector']==9 and r['selected']>=0]
    assert len(all_dma)==int(mark[5]),'AUDIO / DMA TRACE EXTENT'
    assert len(dma)==len(expected_dma),'AUDIO / DMA COVERAGE'
    for actual,(i,row) in zip(dma,expected_dma):
        assert actual[0]==i and actual[1]<4,('AUDIO / DMA ORDER',i,actual)
        assert actual[2:9]==descriptor(row),('AUDIO / DMA DATA',i,actual[2:9],descriptor(row))
        assert actual[9]&(1<<actual[1]) and actual[9]&512 and actual[10]&2 and actual[11]==0,('AUDIO / DMA HARDWARE',i,actual)
    active={};tails=0
    for actual in all_dma:
        i=actual[0];row=original[i]
        if not actual[12]:
            if row['selector']==9:active[row['selected']]=row
        else:
            slot=actual[12]-1
            if row['releases']:assert (slot,actual[13]) in row['releases'],('AUDIO / ORIGINAL TAIL CURSOR',i,slot,actual[13],row['releases'])
            assert row['selector']==12 and slot in active,'AUDIO / TAIL OWNER'
            note=active[slot];assert row['packet'][2:4]==note['packet'][2:4] and row['packet'][8:10]==note['packet'][8:10],'AUDIO / RELEASE NOTE'
            assert actual[2:9]==descriptor(note,actual[13]),('AUDIO / TAIL DMA',i)
            assert actual[9]&(1<<actual[1]) and actual[10]&2 and actual[11]==0,'AUDIO / TAIL HARDWARE'
            tails+=1
    assert tails>0,'AUDIO / TAIL NOT REACHED'
    assert observed[-1][4]>0,'AUDIO / CALLBACK NOT REACHED'
    if original[-1]['callbacksAfter']:assert observed[-1][4]==original[-1]['callbacksAfter'],'AUDIO / ORIGINAL CALLBACK COUNT'
    phases={p:sum(r['selector']==9 for r in original if r['phase']==p) for p in range(4)}
    print('AUDIO NATIVE PAIR PASS:',len(original),'calls;',len(all_dma),'Paula starts;',tails,'release tails;',observed[-1][4],'callbacks; notes by phase',phases)
def startup(run,reference,setup="a1200-fast"):
    calls(reference)  # Requires a completed, byte-guarded original route.
    log=(run/'gdb-out.log').read_text();restored(run,log,setup)
    assert 'CORE STOP trap=aaa0 segment=17 offset=0820 count=144\n' in log,'AUDIO / STARTUP BOUNDARY'
    assert 'CORE REASON UI / STARTUP CHECKPOINT\n' in log and 'CORE EXCEPTION vector=0 pc=00000000\n' in log
    assert 'INPUT RESTORE vector=1\n' in log
    actual=re.findall(r'^CODE \d+\+[0-9A-F]+ trap=[0-9A-F]+$',log,re.M)
    expected=[line.split(' selector=')[0] for line in (reference/'traps.txt').read_text().splitlines()[:144]]
    assert actual==expected,'AUDIO / STARTUP REGRESSION'
    print('AUDIO STARTUP PASS: 144 original calls; Palette2CTab checkpoint; restored OS')

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run',type=Path)
    parser.add_argument('--startup',type=Path)
    parser.add_argument('--setup',choices=('a1200-fast','a1200-030','a1200-040'),default='a1200-fast')
    args=parser.parse_args()
    if args.startup:startup(args.run,args.startup,args.setup)
    else:check(args.run,args.setup)
