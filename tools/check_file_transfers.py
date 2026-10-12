#!/usr/bin/env python3
"""Replay positively checked original transfers against private host file copies."""
import argparse
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
from extract_original_data import ROOT, DESTINATION
from report_files import report


def check(reference):
    rows=report(reference)
    scratch=Path(tempfile.mkdtemp(prefix='file-transfers-',dir=ROOT/'tmp'))
    runtime=scratch/'runtime';shutil.copytree(DESTINATION,runtime)
    lines=[]
    for row in rows:
        serial=row['id'];trap=int(row['trap'],16);ref=row['ref']
        before=(reference/f'file-{serial}-before.bin').read_bytes()
        after=(reference/f'file-{serial}-after.bin').read_bytes()
        opening=trap in (0xa000,0xa200) or trap in (0xa060,0xa260) and int(row['selector'],16)==26
        if opening and not row['error']:
            path=runtime/'Data'/row['name'].removeprefix(':Data:')
            lines.append(f'O {row["returned_ref"]} {json.dumps(str(path))} {before[27]}')
        elif trap==0xa001:lines.append(f'C {ref}')
        elif trap==0xa011:lines.append(f'E {ref} {struct.unpack_from(">I",after,28)[0]}')
        elif trap==0xa044:
            mode=struct.unpack_from('>H',before,44)[0];offset=struct.unpack_from('>i',before,46)[0]
            position=struct.unpack_from('>i',after,46)[0]
            lines.append(f'S {ref} {mode} {offset} {position}')
        elif trap in (0xa002,0xa003):
            op='R' if trap==0xa002 else 'W';label='transfer' if op=='R' else 'requested'
            payload=reference/f'file-{serial}-{label}.bin'
            lines.append(f'{op} {ref} {row["mode"]} {row["offset"]} {row["request"]} {row["actual"]} {row["error"]} {row["position"]} {json.dumps(str(payload))}')
    script=scratch/'replay.txt';script.write_text('\n'.join(lines)+'\n')
    exe=scratch/'replay'
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-O2','-include','cstdint','-Isrc',
                    'tools/file_transfer_check.cpp','src/mac/MacFiles.cpp','src/mac/FileMetadata.cpp',
                    'src/mac/FileTransfers.cpp','-o',str(exe)],cwd=ROOT,check=True)
    subprocess.run([str(exe),str(script)],check=True)
    print('FILE TRANSFER EVIDENCE:',scratch.relative_to(ROOT))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('reference',type=Path)
    check(parser.parse_args().reference.resolve())
