#!/usr/bin/env python3
"""Pair the bounded native resource probe with completed original resource calls."""
import argparse
import re
import struct
from pathlib import Path
from check_core_result import check
from report_resources import report


def verify(directory,reference,core_reference):
    check(directory,core_reference)
    rows=report(reference)
    log=(directory/'gdb-out.log').read_text()
    probes=re.findall(r'^RESOURCE PROBE shape=([0-9a-f]+) option=([0-9a-f]+) refs=(\d+)/(\d+)/(\d+)/(\d+)$',log,re.M)
    if len(probes)!=1:raise ValueError('NATIVE RESOURCE / COMPLETION')
    code_name=re.findall(r'^RESOURCE CODE NAME address=([0-9a-f]+)$',log,re.M)
    code2=re.findall(r'^CORE SEGMENT id=2 base=([0-9a-f]+) size=29248$',log,re.M)
    if len(code_name)!=1 or len(code2)!=1 or int(code_name[0],16)!=int(code2[0],16)+0x4284:
        raise ValueError('NATIVE RESOURCE / ORIGINAL CODE NAME ADDRESS')
    original_log=(reference/'session.log').read_text()
    original_code2=re.findall(r'^MAP seg=2 base=([0-9A-F]+) size=29248$',original_log,re.M)
    original_name=int.from_bytes((reference/'resource-23-before.bin').read_bytes()[:4],'big')
    if not original_code2 or original_name-int(original_code2[0],16)!=0x4284:
        raise ValueError('NATIVE RESOURCE / ORIGINAL NAME LOCATION EVIDENCE')
    shape,option,*refs=probes[0]
    if not int(shape,16) or not int(option,16) or len(set(refs))!=4 or '0' in refs:
        raise ValueError('NATIVE RESOURCE / HANDLES AND FILE REFS')
    for label,kind,rid in [('shape','SHAP',25002),('option','DSLV',6400)]:
        matches=[r for r in rows if r['type']==kind and r['rid']==rid]
        if not matches:raise ValueError('NATIVE RESOURCE / REFERENCE COVERAGE')
        body=(directory/f'resource-{label}.bin').read_bytes()
        for row in matches:
            if body!=(reference/f'resource-{row["id"]}-payload.bin').read_bytes():
                raise ValueError('NATIVE RESOURCE / PAIRED BODY')
    info=(directory/'resource-info.bin').read_bytes()
    if len(info)!=264 or info[:5]!=b'\x04DSLV' or info[256:264]!=struct.pack('>h2x4s',6400,b'DSLV'):
        raise ValueError('NATIVE RESOURCE / METADATA')
    specs=(directory/'file-specs.bin').read_bytes()
    results=re.findall(r'^SPEC PROBE index=(\d+) result=(-?\d+)$',log,re.M)
    calls=(30,31,33,35,36,44,45,50,51,54,55)
    if len(specs)!=70*len(calls) or [int(i) for i,_ in results]!=list(range(len(calls))):
        raise ValueError('NATIVE FSSPEC / COMPLETION')
    parent=lambda serial:int.from_bytes((reference/f'resource-{serial}-spec-after.bin').read_bytes()[2:6],'big')
    parents={parent(30):2,parent(33):3,parent(50):4}
    for index,serial in enumerate(calls):
        expected=(reference/f'resource-{serial}-spec-after.bin').read_bytes()
        expected_error=struct.unpack_from('>h',(reference/f'resource-{serial}-after.bin').read_bytes())[0]
        actual=specs[index*70:(index+1)*70]
        if int(results[index][1])!=expected_error or actual[:2]!=expected[:2] or int.from_bytes(actual[2:6],'big')!=parents[parent(serial)]:
            raise ValueError('NATIVE FSSPEC / PAIRED RESULT OR PARENT')
        if actual[6:7+actual[6]]!=expected[6:7+expected[6]]:
            raise ValueError('NATIVE FSSPEC / PAIRED CANONICAL NAME')
    print('NATIVE RESOURCE PROBE PASS: complete original SHAP and DSLV bytes, metadata, 11 paired FSSpec cases, detach/close retention and core restoration')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory',type=Path);parser.add_argument('reference',type=Path)
    parser.add_argument('core_reference',type=Path)
    args=parser.parse_args();verify(args.directory.resolve(),args.reference.resolve(),args.core_reference.resolve())
