"""Require transparent wrapper continuations on Macintosh and Amiga CPUs."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import struct
import tempfile
from extract_original_data import ROOT,pins
from native_machine import check as check_machine


def expected():
    rows=[]
    for which in range(10):
        for pattern,seed in enumerate((0x11220000,0x89ab0000)):
            for cc in range(32):
                rows.append((which,pattern,cc,*[seed+i*0x101 for i in range(15)],
                             cc,0,seed if which in (5,6,7) else 0,which,1))
    return rows


def original(path):
    text=(path/'session.log').read_text()
    assert text.count('WRAPPER_FIXTURE_COMPLETE cases=640\n')==1,'WRAPPER / ORIGINAL COMPLETION'
    assert text.count('WRAPPER_FIXTURE_RESTORED\n')==1,'WRAPPER / ORIGINAL RESTORATION'
    assert 'FAIL REFERENCE' not in text,'WRAPPER / ORIGINAL FAILURE'
    scoped=text[text.index('WRAPPER_ROW '):]
    assert 'unmapped' not in scoped,'WRAPPER / FIXTURE MEMORY ACCESS'
    return [tuple(int(v,16) for v in line.split(',')) for line in re.findall(r'^WRAPPER_ROW ([0-9A-F,]+)$',text,re.M)]


def native(path):
    check_machine(path,'a1200-030')
    receipt=json.loads((path/'receipt.json').read_text())
    assert receipt['native_source']==hashlib.sha256((ROOT/'tools/wrapper_ccr_native.c').read_bytes()).hexdigest(),'WRAPPER / NATIVE SOURCE'
    for name,key in [('dh1/WrapperProbe','exe'),('fixture.elf','elf')]:
        assert hashlib.sha256((path/name).read_bytes()).hexdigest()==receipt[key],'WRAPPER / EXECUTABLE IDENTITY'
    fixture=(ROOT/receipt['fixture']).resolve()
    recorded=pins(ROOT/'manifest/wrapper-ccr-inputs.sha256')
    for name in ('image.bin','metadata.json','fixture.h'):
        file=fixture/name;key=str(file.relative_to(ROOT))
        assert key in recorded and hashlib.sha256(file.read_bytes()).hexdigest()==recorded[key],'WRAPPER / FIXTURE INPUT PIN'
    meta=json.loads((fixture/'metadata.json').read_text())
    assert meta['stub_sha256']==hashlib.sha256((ROOT/'tools/wrapper_ccr_fixture.s').read_bytes()).hexdigest(),'WRAPPER / CALLBACK SOURCE'
    assert receipt['image']==meta['image_sha256'],'WRAPPER / IMAGE IDENTITY'
    assert (path/'dh1/Shell.done').read_text()=='SHELL PASS\n','WRAPPER / NATIVE SHELL'
    assert (path/'dh1/fixture.complete').read_text()=='WRAPPER_FIXTURE_COMPLETE cases=640\n','WRAPPER / NATIVE COMPLETION'
    data=(path/'dh1/wrapper-rows.bin').read_bytes()
    assert len(data)==640*23*4,'WRAPPER / NATIVE EXTENT'
    return list(struct.iter_unpack('>23I',data))


def compare(rows):
    wanted=expected()
    assert len(rows)==len(wanted),'WRAPPER / CASE COUNT'
    for index,(actual,expect) in enumerate(zip(rows,wanted)):
        assert actual==expect,('WRAPPER / CONTINUATION',index,actual,expect)


def controls(reference,port):
    baseline=original(reference);compare(baseline)
    rejected=0
    def reject(fn):
        nonlocal rejected
        try:fn()
        except (AssertionError,RuntimeError,ValueError):rejected+=1
        else:raise AssertionError('WRAPPER / INACTIVE REJECTION CONTROL')
    for column in range(23):
        changed=list(baseline);row=list(changed[0]);row[column]^=1;changed[0]=tuple(row)
        reject(lambda:compare(changed))
    reject(lambda:compare(baseline[:-1]))
    with tempfile.TemporaryDirectory(prefix='wrapper-controls-',dir=ROOT/'tmp') as directory:
        scratch=Path(directory);log=scratch/'session.log';good=(reference/'session.log').read_text()
        for old,new in [('WRAPPER_FIXTURE_COMPLETE cases=640','INCOMPLETE'),
                        ('WRAPPER_FIXTURE_RESTORED','MISSING RESTORATION')]:
            assert old in good;log.write_text(good.replace(old,new));reject(lambda:original(scratch))
        log.write_text(good+'unmapped fixture memory access\n');reject(lambda:original(scratch))
        for name in ('receipt.json','fixture.elf','dh1/WrapperProbe','dh1/Shell.done',
                     'dh1/fixture.complete','dh1/wrapper-rows.bin','logs/debug.uae'):
            target=scratch/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(port/name,target)
        compare(native(scratch))
        for name,bad in [('dh1/Shell.done',b'SHELL FAIL\n'),('dh1/fixture.complete',b'incomplete\n'),
                         ('dh1/wrapper-rows.bin',b''),('dh1/WrapperProbe',b'changed executable')]:
            target=scratch/name;good_bytes=target.read_bytes();target.write_bytes(bad)
            reject(lambda:native(scratch));target.write_bytes(good_bytes)
        target=scratch/'logs/debug.uae';good_config=target.read_text()
        assert 'cpu_model=68030' in good_config
        target.write_text(good_config.replace('cpu_model=68030','cpu_model=68040'))
        reject(lambda:native(scratch))
        target.write_text(good_config)
        target=scratch/'receipt.json';receipt=json.loads(target.read_text())
        receipt['fixture']='tmp/unpinned-wrapper-fixture';target.write_text(json.dumps(receipt))
        reject(lambda:native(scratch))
    return rejected


def verify_evidence(paths):
    recorded=pins(ROOT/'manifest/wrapper-ccr-evidence.sha256')
    for path in paths:
        key=str(path.resolve().relative_to(ROOT))
        assert key in recorded and hashlib.sha256(path.read_bytes()).hexdigest()==recorded[key],'WRAPPER / EVIDENCE PIN '+key


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--original',type=Path,required=True)
    p.add_argument('--native',type=Path);p.add_argument('--controls',action='store_true');a=p.parse_args()
    if a.controls and not a.native:p.error('--controls requires --native')
    verify_evidence([a.original/'session.log'])
    reference=original(a.original);compare(reference)
    if a.native:
        verify_evidence([a.native/name for name in ('receipt.json','fixture.elf','dh1/WrapperProbe',
            'dh1/Shell.done','dh1/fixture.complete','dh1/wrapper-rows.bin','logs/debug.uae')])
        port=native(a.native);compare(port)
        assert port==reference,'WRAPPER / ORIGINAL PAIR'
        receipt=json.loads((a.native/'receipt.json').read_text())
        meta=json.loads((ROOT/receipt['fixture']/'metadata.json').read_text())
        assert meta['source_sha256']==hashlib.sha256((ROOT/'src/platform/amiga/MacEntry.s').read_bytes()).hexdigest(),'WRAPPER / CURRENT SOURCE'
    if a.controls:print('WRAPPER CONTROLS PASS:',controls(a.original,a.native),'rejected alterations')
    print('WRAPPER CCR PASS: 640 actual RTS continuations; 10 wrappers, all 32 CCR states, 15 registers, stack balance/canaries and callback arguments')
