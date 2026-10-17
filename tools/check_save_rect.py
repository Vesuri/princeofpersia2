"""Pair complete SaveRect loop effects with Macintosh execution, not an emulation."""
import argparse
import hashlib
import json
from pathlib import Path
from extract_original_data import ROOT,pins
from native_machine import check as check_machine
from save_rect_fixture import verify_fixture


def compare(data,expected,cases):
    assert len(data)==len(expected),'SAVE RECT / OUTPUT EXTENT'
    at=0
    for index,(width,src,dst,cc) in enumerate(cases):
        end=at+132+width+max(src,dst)+16
        assert data[at:end]==expected[at:end],('SAVE RECT / EFFECT MISMATCH',index,width,src,dst,cc)
        at=end
    assert at==len(data),'SAVE RECT / CASE EXTENT'


def check(reference,native):
    log=(reference/'session.log').read_text()
    assert log.count('SAVE_RECT_COMPLETE cases=672\n')==1,'SAVE RECT / ORIGINAL COMPLETION'
    assert log.count('SAVE_RECT_RESTORED\n')==1 and 'FAIL REFERENCE' not in log,'SAVE RECT / ORIGINAL RESTORATION'
    assert 'unmapped' not in log[log.index('SAVE_RECT_ALLOC\n'):],'SAVE RECT / ORIGINAL INVALID ACCESS'
    check_machine(native,'a1200-fast')
    receipt=json.loads((native/'receipt.json').read_text())
    fixture=ROOT/receipt['fixture'];verify_fixture(fixture)
    cases=json.loads((fixture/'cases.json').read_text());assert len(cases)==672
    assert receipt['image']==hashlib.sha256((fixture/'original.bin').read_bytes()).hexdigest()
    assert receipt['native_source']==hashlib.sha256((ROOT/'tools/save_rect_native.c').read_bytes()).hexdigest()
    for name,digest in receipt['sources'].items():assert hashlib.sha256((ROOT/name).read_bytes()).hexdigest()==digest,('SAVE RECT / SOURCE',name)
    assert set(receipt['sources'])=={'tools/wrapper_ccr_fixture.s','src/platform/amiga/SaveRect.s','src/mac/ForwardLongCopy.inc'}
    for name,key in (('fixture.elf','elf'),('dh1/WrapperProbe','exe')):
        assert hashlib.sha256((native/name).read_bytes()).hexdigest()==receipt[key],('SAVE RECT / EXECUTABLE',name)
    assert (native/'dh1/fixture.complete').read_text()=='SAVE_RECT_COMPLETE\n'
    assert (native/'dh1/Shell.done').read_text()=='SHELL PASS\n'
    expected=(reference/'save-rect.bin').read_bytes()
    for name in ('original.bin','replacement.bin'):
        compare((native/'dh1'/name).read_bytes(),expected,cases)
    # Corrupt every register, CCR, SP, the copied bytes and untouched frame locals.
    rejected=0
    for offset in [*range(0,68,4),60,61,68,120,170,len(expected)-1]:
        bad=bytearray(expected);bad[offset]^=1
        try:compare(bad,expected,cases)
        except AssertionError:rejected+=1
        else:raise AssertionError('SAVE RECT / INACTIVE DIFFERENTIAL CONTROL')
    try:compare(expected[:-1],expected,cases)
    except AssertionError:rejected+=1
    else:raise AssertionError('SAVE RECT / INACTIVE EXTENT CONTROL')
    print('SAVE RECT PASS: 672 original-Mac/native-original/native-replacement cases;',rejected,'rejections; full buffer, frame locals, registers, CCR and stack')


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('reference',type=Path);p.add_argument('native',type=Path)
    a=p.parse_args()
    recorded=pins(ROOT/'manifest/save-rect-evidence.sha256')
    paths=[a.reference/name for name in ('session.log','save-rect.bin')]
    paths += [a.native/name for name in ('receipt.json','fixture.elf','dh1/WrapperProbe','dh1/original.bin','dh1/replacement.bin','dh1/fixture.complete','dh1/Shell.done','logs/debug.uae')]
    for path in paths:
        key=str(path.resolve().relative_to(ROOT))
        assert key in recorded and hashlib.sha256(path.read_bytes()).hexdigest()==recorded[key],('SAVE RECT / EVIDENCE PIN',key)
    check(a.reference,a.native)
