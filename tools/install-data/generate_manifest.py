"""Generate names/extents/hashes only for the pinned plain StuffIt 5 archive."""
import hashlib
import json
import struct
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from extract_original_data import ARCHIVE, DESTINATION, ROOT, verify_archive, verify_output

def generate():
    verify_archive(ARCHIVE); hashes=verify_output(DESTINATION)
    data=ARCHIVE.read_bytes()
    u16=lambda p:struct.unpack_from('>H',data,p)[0]
    u32=lambda p:struct.unpack_from('>I',data,p)[0]
    pos=u32(94); rows=[];seen=set()
    while pos<len(data):
        assert u32(pos)==0xa5a5a5a5
        hs=u16(pos+6); flags=data[pos+9]; nl=u16(pos+30)
        if flags&64 and u32(pos+34)==0xffffffff:
            pos+=48;continue
        name=data[pos+48:pos+48+nl].rstrip(b'\0').decode('ascii')
        extra=36 if data[pos+4]==1 else 32
        meta=pos+hs;off=meta+extra;rc=0;resource=None
        if u16(meta)&1:
            rc=u32(off+4)
            resource=(off+14,rc,u32(off),data[off+12],u16(off+8))
            off+=14
        dc=0 if flags&64 else u32(pos+38)
        if not flags&64:
            name=name if name=='Prince of Persia 2' else 'Data/'+name
            forks=[(name,off+rc,dc,u32(pos+34),data[pos+46],u16(pos+42))]
            if resource:forks.append((name+'.rsrc',*resource))
            for n,a,p,s,m,c in forks:
                assert n in hashes and n not in seen and (DESTINATION/n).stat().st_size==s
                seen.add(n);rows.append((n,a,p,s,m,c,hashes[n]))
            n=name+'.finfo';seen.add(n)
            # Metadata reads Finder bytes at entry header+4 and dates at entry+10.
            rows.append((n,pos,meta,32,255,0,hashes[n]))
        pos=off+rc+dc
    assert seen==set(hashes)
    lines=['/* Names, offsets, sizes and hashes only; no game bytes. */',
           'typedef struct { const char *name; uint32_t off,packed,size; unsigned method,crc; const char *hash; } Original;',
           'static const Original originals[]={']
    for n,a,p,s,m,c,h in sorted(rows):lines.append('{'+f'{json.dumps(n)},{a}UL,{p}UL,{s}UL,{m},{c},{json.dumps(h)}'+'},')
    for i,(directory,manifest,name) in enumerate([('core','core-inputs','relocations.bin'),('core','core-inputs','file-catalog.bin'),('ui','ui-inputs','ui-seams.bin'),('audio','audio-inputs','audio-pitch.bin')]):
        from extract_original_data import pins
        p=ROOT/'build'/directory/name;h=pins(ROOT/'manifest'/f'{manifest}.sha256')[name]
        assert hashlib.sha256(p.read_bytes()).hexdigest()==h
        lines.append('{'+f'{json.dumps(name)},{i}UL,0UL,{p.stat().st_size}UL,254,0,{json.dumps(h)}'+'},')
    lines+=['};','#define ORIGINAL_COUNT (sizeof(originals)/sizeof(originals[0]))']
    return '\n'.join(lines)+'\n'
if __name__=='__main__':
    out=Path(__file__).with_name('originals.h');text=generate()
    if '--check' in sys.argv:assert out.read_text()==text
    else:out.write_text(text)
