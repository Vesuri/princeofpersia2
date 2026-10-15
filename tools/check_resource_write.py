"""Compare the persisted dissolve-cache map with the completed Macintosh write."""
import argparse
import hashlib
import re
import struct
from pathlib import Path
from amigaxdev.mac.resource_fork import read_resource_fork
from extract_original_data import ROOT, pins
from check_core_result import restored


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
    assert actual.keys()==expected.keys(), 'RESOURCE WRITE / PERSISTED MAP OR PAYLOAD'
    for key,(attrs,name,body) in expected.items():
        got_attrs,got_name,got_body=actual[key]
        assert (got_attrs,got_name,len(got_body))==(attrs,name,len(body)), 'RESOURCE WRITE / PERSISTED MAP OR PAYLOAD'
        if key==(b'DSLV',6401):
            # DERIVED: MakeDisolveData leaves bytes 12..19 uninitialized.
            # InitDissolve publishes at CODE 15+2D0E before SetDisolveDelays
            # at +2D1C writes both timing fields (+3C4A/+3C54). Compare all
            # initialized bytes, without imposing either heap's old contents.
            assert got_body[:12]==body[:12] and got_body[20:]==body[20:], 'RESOURCE WRITE / PERSISTED MAP OR PAYLOAD'
        else:
            assert got_body==body, 'RESOURCE WRITE / PERSISTED MAP OR PAYLOAD'
    print('RESOURCE WRITE FILE PASS: original-paired map and initialized payloads (DSLV 6401 pre-calibration bytes 12..19 excluded); boot/load still requires live evidence')


def check_readback(native, run):
    """Check the bounded fresh-boot DSLV 6401 resource fixture on A1200-030."""
    log=(run/'gdb-out.log').read_text()
    assert 'CORE STOP trap=a86e segment=2 offset=3e90 count=30' in log, 'RESOURCE READBACK / FIXTURE STOP'
    assert 'CORE REASON UNIMPLEMENTED / InitGraf' in log, 'RESOURCE READBACK / FIXTURE REASON'
    restored(run,log,setup='a1200-030')
    rows=re.findall(r'^RESOURCE PROBE shape=([0-9a-f]+) option=([0-9a-f]+) refs=(\d+)/(\d+)/(\d+)/(\d+)$',log,re.M)
    assert len(rows)==1 and all(int(v,16) for v in rows[0][:2]), 'RESOURCE READBACK / COMPLETION'
    assert len(set(rows[0][2:]))==4 and all(int(v)>0 for v in rows[0][2:]), 'RESOURCE READBACK / FILE REFS'
    info=(run/'resource-info.bin').read_bytes()
    assert len(info)==264 and info[:5]==b'\x04DSLV' and info[256:]==struct.pack('>h2x4s',6401,b'DSLV'), 'RESOURCE READBACK / METADATA'
    expected=next(r.body for r in read_resource_fork(native) if r.kind==b'DSLV' and r.rid==6401)
    assert (run/'resource-option.bin').read_bytes()==expected, 'RESOURCE READBACK / COMPLETE PAYLOAD'
    print('RESOURCE READBACK PASS: fresh A1200-030 boot, complete DSLV 6401 bytes and metadata, fixture stop and restoration')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('native',type=Path)
    parser.add_argument('--original',type=Path,default=ROOT/'tmp/resource-write-original')
    parser.add_argument('--manifest',type=Path,default=ROOT/'manifest/resource-write-output.sha256')
    parser.add_argument('--readback',type=Path,help='fresh-boot POP2_RESOURCE_CACHE_PROBE run')
    args=parser.parse_args();check(args.native,args.original,args.manifest)
    if args.readback:check_readback(args.native,args.readback)
