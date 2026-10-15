"""Reject a no-op publication and a corrupted generated cache."""
from pathlib import Path
import tempfile
import shutil
import unittest
from amigaxdev.mac.resource_fork import read_resource_fork
from check_resource_write import ROOT, check, check_readback


class ResourceWrite(unittest.TestCase):
    def test_original_persisted_file(self):
        check(ROOT/'tmp/resource-write-original/Prince2.opt.rsrc')

    def test_no_write(self):
        with self.assertRaises(AssertionError):
            check(ROOT/'tmp/runtime-data/Data/Prince2.opt.rsrc')

    def test_corrupted_cache(self):
        source=ROOT/'tmp/resource-write-original/Prince2.opt.rsrc'
        payload=next(r.body for r in read_resource_fork(source) if r.kind==b'DSLV' and r.rid==6401)
        body=bytearray(source.read_bytes());at=body.find(payload)
        self.assertGreaterEqual(at,0);body[at+len(payload)//2]^=1
        with tempfile.TemporaryDirectory(dir=ROOT/'tmp',prefix='resource-write-control-') as folder:
            changed=Path(folder)/'changed.rsrc';changed.write_bytes(body)
            with self.assertRaises(AssertionError):check(changed)

    def test_precalibration_fields_only(self):
        source=ROOT/'tmp/resource-write-original/Prince2.opt.rsrc'
        payload=next(r.body for r in read_resource_fork(source) if r.kind==b'DSLV' and r.rid==6401)
        body=bytearray(source.read_bytes());at=body.find(payload)
        self.assertGreaterEqual(at,0)
        with tempfile.TemporaryDirectory(dir=ROOT/'tmp',prefix='resource-write-control-') as folder:
            changed=Path(folder)/'changed.rsrc'
            body[at+12:at+20]=bytes(8);changed.write_bytes(body);check(changed)
            for offset in (0,11,20,len(payload)-1):
                corrupt=body.copy();corrupt[at+offset]^=1;changed.write_bytes(corrupt)
                with self.assertRaises(AssertionError):check(changed)

    def test_fresh_boot_readback_controls(self):
        source=ROOT/'amiga/.run-ZTGAT7'
        native=ROOT/'amiga/.run-dyQmd3/dh1/Data/Prince2.opt.rsrc'
        check_readback(native,source)
        names=('gdb-out.log','resource-info.bin','resource-option.bin','dh1/Shell.done','logs/debug.uae')
        with tempfile.TemporaryDirectory(dir=ROOT/'tmp',prefix='resource-readback-control-') as folder:
            run=Path(folder)
            for name in names:
                target=run/name;target.parent.mkdir(parents=True,exist_ok=True)
                shutil.copyfile(source/name,target)
            for name,changed in (
                ('resource-option.bin',b''),
                ('resource-info.bin',bytes(264)),
                ('gdb-out.log',(source/'gdb-out.log').read_bytes().replace(b'RESOURCE PROBE',b'UNREACHED PROBE')),
                ('dh1/Shell.done',b'NOT COMPLETE'),
                ('logs/debug.uae',b'cpu_model=68040\n'),
            ):
                (run/name).write_bytes(changed)
                with self.assertRaises((AssertionError,RuntimeError)):check_readback(native,run)
                shutil.copyfile(source/name,run/name)


if __name__=='__main__':unittest.main()
