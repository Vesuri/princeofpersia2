"""Compare the persisted dissolve-cache map with the completed Macintosh write."""
import argparse
import hashlib
from pathlib import Path
from amigaxdev.mac.resource_fork import read_resource_fork
from extract_original_data import ROOT, pins


def check(native, original=ROOT/'tmp/resource-write-original', manifest=ROOT/'manifest/resource-write-output.sha256'):
    for name, expected in pins(manifest).items():
        assert hashlib.sha256((original/name).read_bytes()).hexdigest()==expected, 'RESOURCE WRITE / ORIGINAL HASH'
    def records(path):
        rows=read_resource_fork(path)
        result={(r.kind,r.rid):(r.attrs,r.name,r.body) for r in rows}
        assert len(result)==len(rows), 'RESOURCE WRITE / DUPLICATE ID'
        return result
    expected=records(original/'Prince2.opt.rsrc')
    assert set(expected)=={(b'DSLV',6400),(b'DSLV',6401)}, 'RESOURCE WRITE / ORIGINAL CACHE MEMBERS'
    assert len(expected[(b'DSLV',6401)][2])==373788, 'RESOURCE WRITE / ORIGINAL CACHE SIZE'
    actual=records(native)
    assert actual==expected, 'RESOURCE WRITE / PERSISTED MAP OR PAYLOAD'
    print('RESOURCE WRITE FILE PASS: original-paired names, attributes and both DSLV payloads; boot/load still requires live evidence')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('native',type=Path)
    parser.add_argument('--original',type=Path,default=ROOT/'tmp/resource-write-original')
    parser.add_argument('--manifest',type=Path,default=ROOT/'manifest/resource-write-output.sha256')
    args=parser.parse_args();check(args.native,args.original,args.manifest)
