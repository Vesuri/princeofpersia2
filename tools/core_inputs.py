#!/usr/bin/env python3
"""Generate verified runtime relocation inputs; original bytes remain ignored."""
import hashlib
import struct
from static_map import build, ROOT, DESTINATION
from extract_original_data import pins
import trap_census as census

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
