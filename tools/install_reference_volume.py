#!/usr/bin/env python3
"""Install verified forks on a fresh HFS volume, adapted from Alone in the Dark."""
import argparse
from pathlib import Path
import struct
import subprocess

from extract_original_data import APPLICATION, DESTINATION, scratch_directory, verify_output


def crc16(data):
    crc = 0
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            crc = ((crc << 1) ^ (0x1021 if crc & 0x8000 else 0)) & 0xffff
    return crc


def macbinary(source):
    data = source.read_bytes()
    resource = Path(str(source)+'.rsrc').read_bytes()
    info = Path(str(source)+'.finfo').read_bytes()
    name = source.name.encode('mac_roman')
    header = bytearray(128)
    header[1] = len(name)
    header[2:2+len(name)] = name
    header[65:73] = info[4:12]
    header[73] = info[12]
    header[75:81] = info[14:20]
    header[101] = info[13]
    struct.pack_into('>II', header, 83, len(data), len(resource))
    header[91:99] = info[20:28]
    header[122] = header[123] = 129
    struct.pack_into('>H', header, 124, crc16(header[:124]))
    pad = lambda b: b + bytes(-len(b) % 128)
    return bytes(header) + pad(data) + pad(resource)


def hfs(*args):
    return subprocess.check_output(args, text=True, stderr=subprocess.STDOUT)


def install(volume):
    verified = verify_output(DESTINATION)
    scratch = scratch_directory()
    hfs('hmount', str(volume))
    try:
        parent = ':Desktop Folder' if 'Desktop Folder' in hfs('hls','-1a',':').splitlines() else ''
        base = parent + ':' + APPLICATION
        if APPLICATION in hfs('hls','-1a',parent+':').splitlines():
            raise ValueError('REFERENCE / GAME FOLDER ALREADY EXISTS')
        hfs('hmkdir', base)
        hfs('hmkdir', base+':Data')
        sources = [p[:-6] for p in verified if p.endswith('.finfo')]
        for i, relative in enumerate(sources):
            blob = scratch / (str(i)+'.bin')
            blob.write_bytes(macbinary(DESTINATION / relative))
            target = base + ':' + relative.replace('/',':')
            hfs('hcopy','-m',str(blob),target)
            # MacBinary round-trip independently checks both fork lengths and bytes.
            check = scratch / (str(i)+'-roundtrip.bin')
            hfs('hcopy','-m',target,str(check))
            raw = check.read_bytes()
            original=blob.read_bytes()
            # hcopy clears only Finder's initialized bit so the new folder lays out icons.
            expected=bytearray(original[:128]);expected[73]&=0xfe
            fields=list(range(65,81))+[101]+list(range(91,99))
            if any(raw[j]!=expected[j] for j in fields):
                raise ValueError('REFERENCE / INSTALLED METADATA MISMATCH: '+relative)
            data_size, resource_size = struct.unpack_from('>II',raw,83)
            resource_start = 128 + ((data_size+127)//128)*128
            if (raw[128:128+data_size] != (DESTINATION / relative).read_bytes() or
                raw[resource_start:resource_start+resource_size] !=
                    Path(str(DESTINATION / relative)+'.rsrc').read_bytes()):
                raise ValueError('REFERENCE / INSTALLED FORK MISMATCH: '+relative)
        print('INSTALL PASS: 32 files; 64 forks and Finder metadata verified')
    finally:
        hfs('humount')


if __name__ == '__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('volume', type=Path)
    install(parser.parse_args().volume)
