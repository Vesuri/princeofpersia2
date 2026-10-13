#!/usr/bin/env python3
"""Generate verified runtime relocation inputs; original bytes remain ignored."""
import hashlib
import struct
from static_map import build, ROOT, DESTINATION
from extract_original_data import pins
import trap_census as census

# DERIVED: immediate low-memory pointers followed by indirect reads/writes.
# Coordinates are original CODE resource offsets, not original byte tables.
POINTER_SITES={
 2:(0x23ca,0x23ea,0x23f6,0x3b02,0x3b0a,0x3b8a,0x3ba6,0x4e64,0x4e6c),
 17:(0x4e70,0x4efa),
 21:(0x1488,0x3974,0x3b54,0x3b72,0x3ba2,0x3bc0,0x40ee,0x416c,
     0x42ca,0x44d8,0x4688,0x4aae,0x516c,0x545c,0x5a68,0x6058,0x64d6,0x6842),
}
def pointer_rows():
    rows=[]
    for segment,offsets in POINTER_SITES.items():
        for offset in offsets:
            original=census.CODE[segment][offset:offset+6]
            op,address=struct.unpack('>HI',original)
            assert op&0xf1ff==0x207c and address in (0xab0,0xab4,0x9ee,0xbaa,0x130,0x2aa,0x220,0x11c)
            # 68020 full-format LEA, index suppressed, word base displacement.
            # MOVEA and LEA both preserve CCR; both instructions occupy 6 bytes.
            patched=struct.pack('>HHH',0x41f5|(op&0xe00),0x0160,0x2000+address)
            rows.append(struct.pack('>HHI',segment,6,offset)+original.ljust(12,b'\0')+patched.ljust(12,b'\0'))
    for offset,address,slot in ((0x13a0,0x308,0x1000),(0x105c,0x356,0x1004)):
        original=census.CODE[21][offset:offset+6]
        assert struct.unpack('>HI',original)==(0x203c,address)
        # An A5-shadow cell holds the relocated address. MOVE.L plus NOP
        # preserves the six-byte extent and MOVE's condition-code behavior.
        patched=struct.pack('>HHH',0x202d,0x2000+slot,0x4e71)
        rows.append(struct.pack('>HHI',21,6,offset)+original.ljust(12,b'\0')+patched.ljust(12,b'\0'))
    return rows

def generate():
    mapping=build()
    rows=[]
    for segment,offset,address,_ in mapping['lowmem']:
        # Decode exactly one instruction from verified CODE, independently of
        # listing text. Extension-word position follows the absolute EA.
        from capstone import Cs,CS_ARCH_M68K,CS_MODE_BIG_ENDIAN,CS_MODE_M68K_020
        decoder=Cs(CS_ARCH_M68K,CS_MODE_BIG_ENDIAN|CS_MODE_M68K_020)
        original=bytes(next(decoder.disasm(census.CODE[segment][offset:],offset,count=1)).bytes)
        words=list(struct.unpack('>'+str(len(original)//2)+'H',original))
        op=words[0]
        if op&0x3f==0x38:
            words[0]=(op&~0x3f)|0x2d
            ext=2 if (op&0xff00 in (0x0800,0x0c00,0x0000,0x0200,0x0400,0x0600,0x0a00)) else 1
        elif op>>12 in (1,2,3) and op&0x0fc0==0x01c0:
            words[0]=(op&~0x0fc0)|0x0b40
            ext=len(words)-1
        else:raise ValueError('LOW MEMORY / UNHANDLED ENCODING')
        assert words[ext]==address and 0<=address<0x2000 and len(original)<=12
        words[ext]=0x2000+address
        patched=struct.pack('>'+str(len(words))+'H',*words)
        decoded=list(decoder.disasm(patched,offset,count=1))
        assert len(decoded)==1 and decoded[0].size==len(original) and 'a5' in decoded[0].op_str
        rows.append(struct.pack('>HHI',segment,len(original),offset)+original.ljust(12,b'\0')+patched.ljust(12,b'\0'))
    rows.extend(pointer_rows())
    out=ROOT/'build/core';out.mkdir(parents=True,exist_ok=True)
    plan=struct.pack('>I',len(rows))+b''.join(rows)
    assert hashlib.sha256(plan).hexdigest()==pins(ROOT/'manifest/core-inputs.sha256')['relocations.bin'], 'RELOCATION PLAN PIN'
    (out/'relocations.bin').write_bytes(plan)
    from file_catalog import catalog
    files=catalog()
    assert hashlib.sha256(files).hexdigest()==pins(ROOT/'manifest/core-inputs.sha256')['file-catalog.bin'], 'FILE CATALOG PIN'
    (out/'file-catalog.bin').write_bytes(files)
    (out/'Inputs.h').write_text(header())
    print('CORE INPUTS PASS:',len(rows),'byte-verified low-memory relocations')
def header():
    def array(name,digest):return 'static const uint8_t '+name+'[32]={'+','.join('0x'+digest[i:i+2] for i in range(0,64,2))+'};\n'
    app=pins(ROOT/'manifest/runtime-data.sha256')['Prince of Persia 2.rsrc']
    plan=pins(ROOT/'manifest/core-inputs.sha256')['relocations.bin']
    return '// Generated hashes only. Inputs themselves stay on disk.\n'+array('applicationSHA256',app)+array('relocationsSHA256',plan)+array('fileCatalogSHA256',pins(ROOT/'manifest/core-inputs.sha256')['file-catalog.bin'])

def verify():
    out=ROOT/'build/core'
    if (out/'Inputs.h').read_text()!=header():raise ValueError('CORE / GENERATED HASH HEADER')
    if hashlib.sha256((out/'relocations.bin').read_bytes()).hexdigest()!=pins(ROOT/'manifest/core-inputs.sha256')['relocations.bin']:
        raise ValueError('CORE / RELOCATION PLAN PIN')
    from file_catalog import catalog
    expected=catalog()
    if (out/'file-catalog.bin').read_bytes()!=expected or hashlib.sha256(expected).hexdigest()!=pins(ROOT/'manifest/core-inputs.sha256')['file-catalog.bin']:
        raise ValueError('CORE / FILE CATALOG PIN')
    print('CORE BUILD INPUTS PASS')

if __name__=='__main__':
    import sys
    verify() if '--verify' in sys.argv else generate()
