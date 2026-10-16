"""Pair direct Pascal rectangle entries with the original System 7.5.5 fixture."""
import argparse
import re
import struct
from pathlib import Path

RECTS = [(0,0,10,20),(5,10,15,30),(10,20,20,40),(0,0,0,0),
         (10,20,0,0),(-32768,-32768,32767,32767),(-32768,-1,0,1),(0,-32768,1,32767),(-20,-30,-10,-5),(32767,32767,-32768,-32768)]
ENTRIES = {0xa8aa:'pop2SectRect',0xa8ab:'pop2UnionRect',0xa8ae:'pop2EmptyRect'}

def cases():
    for trap in ENTRIES:
        for a in range(len(RECTS)):
            for b in range(1 if trap==0xa8ae else len(RECTS)):
                for alias in range(1 if trap==0xa8ae else 3):
                    for cc in (0,31):
                        yield trap,a,b,alias,cc

def rows(path):
    text=path.read_text()
    assert text.count('RECTANGLE_COMPLETE calls=1220\n')==1, 'RECTANGLE / COMPLETION'
    assert not any(s in text for s in ('FAIL REFERENCE','RECTANGLE STACK FAIL','RECTANGLE PRESERVED REGISTER FAIL')), 'RECTANGLE / FAILURE'
    result=re.findall(r'^RECTANGLE_RESULT .+$',text,re.M)
    assert len(result)==len(list(cases())), 'RECTANGLE / CASE COUNT'
    for row,(trap,a,b,alias,cc) in zip(result,cases()):
        assert f'trap={trap:04X} a={a+1} b={b+1} alias={alias} cc={cc} ' in row, 'RECTANGLE / CASE ID'
    return result

def compare(expected,actual):
    assert len(expected)==len(actual), 'RECTANGLE / LENGTH'
    for a,b in zip(expected,actual):
        assert a==b, f'RECTANGLE / ORIGINAL MISMATCH\n{a}\n{b}'

def check(original,native):
    from check_core_result import restored
    restored(native,(native/'gdb-out.log').read_text(),'a1200-030')
    import hashlib
    from extract_original_data import ROOT, pins
    import json
    flags=json.loads((native/'build-receipt.json').read_text())['flags'].split()
    ordinary='-DPOP2_RECTANGLE_TRAP_PROBE' in flags
    log=(native/'gdb-out.log').read_text()
    assert log.count(f'RECTANGLE ORDINARY calls={1220 if ordinary else 0}\n')==1, 'RECTANGLE / ENTRY COVERAGE'
    expected=rows(original/'session.log')
    data=(native/'rectangles.bin').read_bytes();assert len(data)==1220*92, 'RECTANGLE / NATIVE EXTENT'
    actual=[]
    for index,(trap,a,b,alias,cc) in enumerate(cases(),1):
        row=data[(index-1)*92:index*92];sr,result=struct.unpack_from('>HH',row)
        rects=','.join(f'{v:04X}' for v in struct.unpack_from('>12H',row,4))
        regs=struct.unpack_from('>15I',row,28)
        assert regs[10:]==tuple(0x22330000+i*0x101 for i in range(2,7)), 'RECTANGLE / PRESERVED A2-A6'
        assert struct.unpack_from('>I',row,88)[0]==0, 'RECTANGLE / PASCAL STACK'
        values=','.join(f'{v:08X}' for v in regs[:8])
        actual.append(f'RECTANGLE_RESULT index={index} trap={trap:04X} a={a+1} b={b+1} alias={alias} cc={cc} sr={sr:02X} result={result:04X} rects={rects} data={values}')
    contract=('\n'.join(expected)+'\n').encode()
    assert pins(ROOT/'manifest/pure-rectangles-reference.sha256') == {'rectangles-contract.txt':hashlib.sha256(contract).hexdigest()}, 'RECTANGLE / ORIGINAL PIN'
    assert (native/'gdb-out.log').read_text().count('RECTANGLE_COMPLETE calls=1220 guards=13\n')==1, 'RECTANGLE / GUARD CONTROLS'
    text=(native/'gdb-out.log').read_text()
    assert text.count('RECTANGLE RESTORED\n')==1, 'RECTANGLE / RESTORATION'
    assert text.count('CORE REASON INTEGRATION / ROUTE COMPLETE\n')==1, 'RECTANGLE / ROUTE COMPLETION'
    compare(expected,actual)
    # Positive comparison plus a deliberately changed result proves the comparator acts.
    changed=actual.copy();changed[0]=changed[0].replace('result=015A','result=005A')
    try:compare(expected,changed)
    except AssertionError:pass
    else:raise AssertionError('RECTANGLE / INACTIVE CONTROL')
    print('PURE RECTANGLES PASS: 1220 original/native calls, signed boundaries, empty/touching/overlap, both input aliases, CCR, data registers, Pascal stack/padding; restored route')

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--original',type=Path,required=True);p.add_argument('--native',type=Path,required=True);a=p.parse_args()
    check(a.original,a.native)
