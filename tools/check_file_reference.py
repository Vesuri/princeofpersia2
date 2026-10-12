#!/usr/bin/env python3
"""Original-only File Manager observer gate; not the item-7 native acceptance gate."""
import argparse
import contextlib
import io
import json
import os
from pathlib import Path
import shutil
import tempfile
from extract_original_data import ROOT
from report_files import report
from run_reference import run


def normalized(rows):
    # Reference numbers and non-selector D0 values are volatile. Preserve the
    # original call site, operation, semantic inputs, errors and byte hashes.
    result=[]
    for row in rows:
        record={k:v for k,v in row.items() if k not in ('ref','returned_ref','selector')}
        if row['trap'] in ('A060','A260'):record['selector']=row['selector']
        result.append(record)
    return result


def check(directories):
    rows=[report(p) for p in directories]
    assert normalized(rows[0])==normalized(rows[1]),'FILE REPEATABILITY'
    scratch=Path(tempfile.mkdtemp(prefix='file-observer-controls-',dir=ROOT/'tmp'))
    source=directories[0]
    for path in source.iterdir():
        if path.name=='session.log' or path.name.startswith('file-') and path.suffix=='.bin':
            shutil.copyfile(path,scratch/path.name)
    report(scratch) # Prove the copied evidence passes before corruption.
    def reject(label,path,mutate):
        original=path.read_bytes()
        try:
            path.write_bytes(mutate(original))
            try:
                with contextlib.redirect_stdout(io.StringIO()):report(scratch)
            except ValueError:
                print('FILE CONTROL PASS:',label)
            else:raise AssertionError('FILE CONTROL ACCEPTED: '+label)
        finally:path.write_bytes(original)
    reject('missing completion',scratch/'session.log',lambda b:b.replace(b'FILE_TRACE_COMPLETE',b'FILE_TRACE_MISSING'))
    read=next(r for r in rows[0] if r['trap']=='A002' and r['actual'])
    write=next(r for r in rows[0] if r['trap']=='A003' and r['actual'])
    reject('changed read bytes',scratch/f'file-{read["id"]}-transfer.bin',lambda b:bytes([b[0]^1])+b[1:])
    reject('short transfer',scratch/f'file-{read["id"]}-transfer.bin',lambda b:b[:-1])
    reject('changed write source',scratch/f'file-{write["id"]}-requested.bin',lambda b:bytes([b[0]^1])+b[1:])
    reject('changed returned position',scratch/f'file-{read["id"]}-after.bin',lambda b:b[:49]+bytes([b[49]^1])+b[50:])
    result={'references':[str(p.relative_to(ROOT)) for p in directories],
            'calls':len(rows[0]),'controls':5,'scope':'original opening file operations only'}
    (scratch/'report.json').write_text(json.dumps(result,indent=2)+'\n')
    print('FILE REFERENCE GATE PASS:',scratch.relative_to(ROOT))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--references',nargs=2,type=Path)
    args=parser.parse_args()
    if args.references:
        directories=[p.resolve() for p in args.references]
    else:
        os.environ['POP2_FILES']='1'
        directories=[run(),run()]
    check(directories)
