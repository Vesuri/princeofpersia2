#!/usr/bin/env python3
"""Validate the bounded native File Manager/DOS-window probe, not the full item-7 gate."""
import argparse
from pathlib import Path
import re
import struct
from report_files import report
from extract_original_data import ROOT, DESTINATION, verify_output, pins
from check_core_result import check


def verify(directory,reference,settings=None):
    if settings not in (None,"write","restart"):raise ValueError("NATIVE FILE / UNKNOWN SETTINGS PROFILE")
    check(directory)
    verify_output(DESTINATION)
    log=(directory/'gdb-out.log').read_text()
    probes=re.findall(r'^FILE PROBE open=(-?\d+) read=(-?\d+) close=(-?\d+) windows=(\d+) fields=(\d+) active=(\d+) actual=(\d+)$',log,re.M)
    if len(probes)!=1:raise ValueError('NATIVE FILE / PROBE COMPLETION')
    opened,read,closed,windows,fields,active,actual=map(int,probes[0])
    if (opened,read,closed,active,actual)!=(0,0,0,0,16) or windows!=(4 if settings=="write" else 3) or fields<1:
        raise ValueError('NATIVE FILE / OPERATION OR SCHEDULING')
    expected=(DESTINATION/'Data/Prince2.opt').read_bytes()
    if settings:
        expected=bytes([expected[0]^1])+expected[1:]
        if (directory/'dh1/Data/Prince2.opt').read_bytes()!=expected:
            raise ValueError('NATIVE FILE / CHANGED SETTINGS ON DISK')
    write_marker='FILE WRITE mode=1 result=0' if settings=='write' else 'FILE WRITE mode=0 result=-1'
    if log.count(write_marker)!=1:raise ValueError('NATIVE FILE / WRITE COMPLETION')
    if (directory/'file-probe.bin').read_bytes()!=expected:raise ValueError('NATIVE FILE / READ BYTES')
    counts=re.findall(r'^FILE VERIFY files=(\d+) bytes=(\d+)$',log,re.M)
    # Startup now hashes the complete installed catalog before timers begin.
    # Rebooted options have changed data and AFI1 metadata; those two mutable
    # files are accepted by the persistence contract, not counted as hash matches.
    originals=pins(ROOT/'manifest/runtime-data.sha256')
    expected_files=1+len(originals)
    expected_bytes=(directory/'dh1/file-catalog.bin').stat().st_size+sum((DESTINATION/name).stat().st_size for name in originals)
    if settings=='restart':
        expected_files-=2
        expected_bytes-=sum((DESTINATION/('Data/Prince2.opt'+suffix)).stat().st_size for suffix in ('','.finfo'))
    if len(counts)!=1 or tuple(map(int,counts[0]))!=(expected_files,expected_bytes):
        raise ValueError('NATIVE FILE / COMPLETE INPUT VERIFICATION')
    if not re.search(r'^FCB PROBE open=0 query=0 close=0$',log,re.M):
        raise ValueError('NATIVE FILE / FCB COMPLETION')
    rows=report(reference)
    opened=next(r for r in rows if r.get('name','').endswith('MIDISnd.dat') and r['error']==0)
    query=next(r for r in rows if r['id']>opened['id'] and r['trap']=='A260' and
               r['selector']=='0008' and r['ref']==opened['returned_ref'])
    original=(reference/f'file-{query["id"]}-after.bin').read_bytes()
    native=(directory/'file-fcb.bin').read_bytes()
    if len(native)!=128 or query['error'] or struct.unpack_from('>h',native,16)[0]:
        raise ValueError('NATIVE FILE / FCB RESULT')
    for offset,size in ((36,2),(40,4),(48,4)):
        if native[offset:offset+size]!=original[offset:offset+size]:
            raise ValueError('NATIVE FILE / PAIRED FCB FLAGS LENGTH OR MARK')
    name=(directory/'file-fcb-name.bin').read_bytes()
    if name[:12]!=b'\x0bMIDISnd.dat':raise ValueError('NATIVE FILE / FCB NAME')
    # IDs and physical allocation belong to different host filesystems. Check
    # native catalog linkage and logical allocation without equating HFS blocks.
    if (int.from_bytes(native[32:36],'big')<5 or int.from_bytes(native[58:62],'big')!=4 or
        native[44:48]!=native[40:44] or native[38:40]!=b'\0\0' or
        native[52:54]!=b'\xff\xff' or native[54:58]!=b'\0'*4):
        raise ValueError('NATIVE FILE / FCB CATALOG LINKAGE')
    print('NATIVE FILE PROBE PASS: '+('changed persisted' if settings else 'original')+' options bytes and MIDI FCB flags/size/position, OS scheduling and restoration')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('directory',type=Path);parser.add_argument('reference',type=Path)
    args=parser.parse_args();verify(args.directory.resolve(),args.reference.resolve())
