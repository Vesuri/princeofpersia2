#!/usr/bin/env python3
"""Deliberately reject incomplete or corrupted original persistence evidence."""
import argparse
import contextlib
import io
from pathlib import Path
import shutil
import tempfile
from extract_original_data import ROOT
from check_file_persistence import check


def controls(written,restarted):
    check(written,restarted)
    scratch=Path(tempfile.mkdtemp(prefix='persistence-controls-',dir=ROOT/'tmp'))
    for path in restarted.iterdir():
        if path.name in ('session.log','opening.ppm') or path.name.startswith('file-') and path.suffix=='.bin':
            shutil.copyfile(path,scratch/path.name)
    shutil.copytree(restarted/'persisted',scratch/'persisted')
    check(written,scratch) # The copied fixture must pass before corruption.
    def reject(label,path,mutate):
        original=path.read_bytes()
        try:
            path.write_bytes(mutate(original))
            try:
                with contextlib.redirect_stdout(io.StringIO()):check(written,scratch)
            except ValueError:print('PERSISTENCE CONTROL PASS:',label)
            else:raise AssertionError('PERSISTENCE CONTROL ACCEPTED: '+label)
        finally:path.write_bytes(original)
    reject('unflushed volume',scratch/'session.log',lambda b:b.replace(b'FILE_ROUTINE_COMPLETE volume-flushed',b'FILE_ROUTINE_INCOMPLETE volume-flushed'))
    reject('resume not completed',scratch/'session.log',lambda b:b.replace(b'FILE_ROUTINE_COMPLETE ResumeGame restart-read',b'FILE_ROUTINE_INCOMPLETE ResumeGame restart-read'))
    reject('changed persisted save',scratch/'persisted/Fixture Save.data',lambda b:bytes([b[0]^1])+b[1:])
    print('PERSISTENCE CONTROLS PASS:',scratch.relative_to(ROOT))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('written',type=Path);parser.add_argument('restarted',type=Path)
    args=parser.parse_args();controls(args.written.resolve(),args.restarted.resolve())
