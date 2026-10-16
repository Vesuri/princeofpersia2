#!/usr/bin/env python3
"""Exact originals, idempotency, preservation, malformed input and cancellation."""
import hashlib
import re
import signal
import subprocess
import sys
import tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
SOURCE=ROOT/'tmp/originals/Prince_Of_Persia2.SIT'
sys.path.insert(0,str(ROOT/'tools'))
from extract_original_data import pins
ORIGINALS=pins(ROOT/'manifest/runtime-data.sha256')
EXPECTED=dict(ORIGINALS)
SUPPORT={}
for directory,manifest,name in [('core','core-inputs','relocations.bin'),('core','core-inputs','file-catalog.bin'),('ui','ui-inputs','ui-seams.bin'),('audio','audio-inputs','audio-pitch.bin')]:
    EXPECTED[name]=pins(ROOT/'manifest'/f'{manifest}.sha256')[name]
    SUPPORT[name]=ROOT/'build'/directory/name


def verify(dest):
    for name,digest in EXPECTED.items():
        assert hashlib.sha256((dest/name).read_bytes()).hexdigest()==digest,name
    assert len([p for p in dest.rglob('*') if p.is_file()])==len(EXPECTED)

def main():
    binary=Path(sys.argv[1]).resolve()
    with tempfile.TemporaryDirectory(prefix='pop2-install-',dir=ROOT/'tmp') as td:
        base=Path(td); scratch=base/'scratch';scratch.mkdir();dest=base/'installed'
        def run(source=SOURCE,good=True):
            r=subprocess.run([binary,source,dest,scratch],capture_output=True,text=True)
            assert (r.returncode==0)==good,r.stdout+r.stderr
            assert not list(scratch.iterdir())
            assert not list(dest.glob('.pop2-*'))
            return r
        run();verify(dest)
        # Independent existing Python extractor output, including every Finder record.
        reference=ROOT/'tmp/runtime-data'
        if reference.exists():
            for name in ORIGINALS:assert (dest/name).read_bytes()==(reference/name).read_bytes(),name
            for name,path in SUPPORT.items():assert (dest/name).read_bytes()==path.read_bytes(),name
        run();verify(dest)
        run(Path('--verify'));verify(dest)
        # Game preferences are mutable; an update must preserve their valid shape.
        opt=dest/'Data/Prince2.opt'; opt.write_bytes(bytes(range(16)))
        run(Path('--verify'));assert opt.read_bytes()==bytes(range(16))
        run(Path('--verify-options'))
        opt.write_bytes(b'invalid')
        run(Path('--verify-options'),False)
        opt.write_bytes((reference/'Data/Prince2.opt').read_bytes())
        # A partially installed tree is repaired without replacing present files.
        keep=dest/'Prince of Persia 2'; stamp=keep.stat().st_mtime_ns
        (dest/'Data/Caverns.rsrc.rsrc').unlink();run();verify(dest)
        assert keep.stat().st_mtime_ns==stamp
        keep.write_bytes(b'preserve unsupported existing original');run(good=False)
        assert keep.read_bytes()==b'preserve unsupported existing original'
        # Run each malformed archive against a new destination.
        original=SOURCE.read_bytes()
        for index,data in enumerate((b'',original[:100],original[:-1],original[:200000]+bytes([original[200000]^128])+original[200001:])):
            dest=base/f'bad-{index}';source=base/'bad.sit';source.write_bytes(data)
            run(source,False);assert not dest.exists()
        dest=base/'cancelled'
        proc=subprocess.Popen([binary,SOURCE,dest,scratch],stdout=subprocess.PIPE,text=True)
        assert 'Extracting and verifying original game files' in proc.stdout.readline()
        proc.send_signal(signal.SIGTERM);out=proc.communicate(timeout=30)[0]
        assert proc.returncode==20 and 'cancelled' in out,out
        assert not dest.exists() and not list(scratch.iterdir())
    print(f'PASS: {len(EXPECTED)} exact files; reference, idempotency, repair, preservation, corruption, cancellation and cleanup')
if __name__=='__main__':main()
