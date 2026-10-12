#!/usr/bin/env python3
"""Require resource evidence verifiers to reject damaged captures."""
import argparse
import contextlib
import io
import json
import shutil
import tempfile
from pathlib import Path
from extract_original_data import ROOT
from report_resources import report
from check_native_resource_probe import verify


def check(native,reference,core_reference):
    verify(native,reference,core_reference)
    scratch=Path(tempfile.mkdtemp(prefix='resource-controls-',dir=ROOT/'tmp'))
    original=scratch/'reference';original.mkdir()
    for p in reference.iterdir():
        if p.name=='session.log' or p.name.startswith('resource-') and p.suffix=='.bin':
            shutil.copyfile(p,original/p.name)
    port=scratch/'native';port.mkdir()
    for p in native.iterdir():
        if p.is_file():shutil.copyfile(p,port/p.name)
    for relative in ('dh1/Shell.done','dh1/relocations.bin','logs/debug.uae'):
        destination=port/relative;destination.parent.mkdir(parents=True,exist_ok=True)
        shutil.copyfile(native/relative,destination)
    # Prove that the copied evidence passes before damaging one field.
    verify(port,original,core_reference)
    passed=[]
    def reject(label,path,mutate,verify):
        before=path.read_bytes()
        try:
            path.write_bytes(mutate(before))
            try:
                with contextlib.redirect_stdout(io.StringIO()):verify()
            except (ValueError,AssertionError):passed.append(label)
            else:raise AssertionError('RESOURCE CONTROL ACCEPTED: '+label)
        finally:path.write_bytes(before)
    reject('original completion',original/'session.log',lambda b:b.replace(b'RESOURCE_TRACE_COMPLETE',b'RESOURCE_TRACE_MISSING'),lambda:report(original))
    reject('original loaded-state evidence',original/'session.log',lambda b:b.replace(b'RESOURCE_BODY id=58 ',b'RESOURCE_BODY_MISSING id=58 '),lambda:report(original))
    flip=lambda b:bytes([b[0]^1])+b[1:]
    reject('original payload',original/'resource-58-payload.bin',flip,lambda:report(original))
    for label,filename,mutate in [
        ('native code-resident name','gdb-out.log',lambda b:b.replace(b'RESOURCE CODE NAME address=',b'RESOURCE CODE NAME missing=')),
        ('native shape','resource-shape.bin',flip),
        ('native full options resource','resource-option.bin',lambda b:b[:-1]),
        ('native resource name','resource-info.bin',lambda b:b[:1]+bytes([b[1]^1])+b[2:]),
        ('native file specification','file-specs.bin',lambda b:b[:7]+bytes([b[7]^1])+b[8:])]:
        reject(label,port/filename,mutate,lambda:verify(port,reference,core_reference))
    (scratch/'report.json').write_text(json.dumps({'controls':passed,'native':str(native.relative_to(ROOT)),
        'reference':str(reference.relative_to(ROOT))},indent=2)+'\n')
    print(f'RESOURCE CONTROLS PASS: {len(passed)}; {scratch.relative_to(ROOT)}')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('native','reference','core_reference'):parser.add_argument(name,type=Path)
    args=parser.parse_args();check(args.native.resolve(),args.reference.resolve(),args.core_reference.resolve())
