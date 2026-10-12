#!/usr/bin/env python3
"""Small controls for flow boundaries, stream extents and transformation semantics."""
import struct
from types import SimpleNamespace
import unittest
import trap_census as c
from static_map import a5world
from driver_image import decrypt, expand

class Checks(unittest.TestCase):
    def test_flow_excludes_inline_trap(self):
        old=c.CODE,c.RELOC,c.JT
        try:
            c.CODE={1:struct.pack('>7H',0,0,0x6004,0xa123,0,0xa9a0,0x4e75)}
            c.RELOC={1:{}};c.JT=[]
            w=c.Walker();w.walk(1,[4])
            self.assertEqual([(s,o,t) for s,o,t,_ in w.traps],[(1,10,0xa9a0)])
            self.assertFalse({6,7,8,9}&w.seen[1])
            self.assertIn(12,w.insn_at[1])
        finally:c.CODE,c.RELOC,c.JT=old

    def test_a5_stream_and_relocation(self):
        def resources(data,zero,drel):
            return [SimpleNamespace(kind=k,rid=0,body=v) for k,v in
                    [(b'CODE',struct.pack('>4I',32,8,0,32)),(b'DATA',data),(b'ZERO',zero),(b'DREL',drel)]]
        rs=resources(bytes.fromhex('fffffff00000'),bytes.fromhex('0002'),struct.pack('>h',-8))
        image,rel,runs=a5world(rs,0x20,0x90)
        self.assertEqual(image,struct.pack('>2I',0x10,0));self.assertEqual((rel,runs),([-8],1))
        for data,zero,drel in [(b'',b'',b''),(b'\0\0',b'\0\x09',b''),(b'12345678',b'',b'\0')]:
            with self.assertRaises(ValueError):a5world(resources(data,zero,drel))

    def test_cipher_round_trip(self):
        clear=bytes(range(64));encrypted=bytearray(clear);state=0xdce5
        for i in range(7,len(clear)):
            encrypted[i]=clear[i]^(state>>8)
            state=((encrypted[i]+state)*0xce6d+0x58bf)&0xffff
        self.assertEqual(decrypt(encrypted),clear)
        self.assertEqual(decrypt(encrypted)[:7],encrypted[:7])

    def test_lzss_overlap_and_rejection(self):
        # Three literals followed by a three-byte overlapping back reference.
        self.assertEqual(expand(struct.pack('>I',6)+bytes([7])+b'abc'+bytes.fromhex('0ffd')),b'abcabc')
        for raw in (struct.pack('>I',3)+b'\0\0\0',struct.pack('>I',2)+b'\x01a'):
            with self.assertRaises(ValueError):expand(raw)

if __name__=='__main__':unittest.main()
