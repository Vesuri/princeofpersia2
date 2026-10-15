"""Reject a no-op publication and a corrupted generated cache."""
from pathlib import Path
import tempfile
import unittest
from amigaxdev.mac.resource_fork import read_resource_fork
from check_resource_write import ROOT, check


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


if __name__=='__main__':unittest.main()
