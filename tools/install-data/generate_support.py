"""Generate installation recipes: original coordinates and authored patch actions only."""
import sys,struct,json,hashlib
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from extract_original_data import ROOT, DESTINATION, verify_output, pins
import _amigaxdev
from amigaxdev.mac.resource_fork import parse_resource_fork
from ui_inputs import SEAMS
from core_inputs import POINTER_SITES, INDIRECT_STORE_SITES

def generate():
    verify_output(DESTINATION)
    raw=(DESTINATION/'Prince of Persia 2.rsrc').read_bytes()
    resources={(r.kind,r.rid):r.body for r in parse_resource_fork(raw)}
    def position(kind,rid):
        body=resources[kind,rid]; at=raw.find(body)
        assert at>=0 and raw.find(body,at+1)<0
        return at
    lines=['/* Original file coordinates and authored transformations; no original bytes. */',
      'typedef struct { unsigned segment,length; uint32_t code_offset,file_offset; unsigned mode; } Relocation;',
      'static const Relocation relocation_recipe[]={']
    plan=(ROOT/'build/core/relocations.bin').read_bytes()
    assert hashlib.sha256(plan).hexdigest()==pins(ROOT/'manifest/core-inputs.sha256')['relocations.bin']
    for i in range(struct.unpack_from('>I',plan)[0]):
        seg,n,off=struct.unpack_from('>HHI',plan,4+32*i)
        mode=3 if (seg,off) in INDIRECT_STORE_SITES else 1 if off in POINTER_SITES.get(seg,()) else 2 if seg==21 and off in (0x13a0,0x105c) else 0
        lines.append('{%d,%d,%dUL,%dUL,%d},'%(seg,n,off,position(b'CODE',seg)+off,mode))
    lines+=['};','typedef struct { unsigned segment,length; uint32_t code_offset,file_offset; unsigned char replacement[12]; } Seam;', 'static const Seam seam_recipe[]={']
    for seg,off,replacement,name in SEAMS:
        lines.append('{%d,%d,%dUL,%dUL,{'%(seg,len(replacement),off,position(b'CODE',seg)+off)+','.join(str(b) for b in replacement)+'}},')
    lines+=['};','#define DRIVER_OFFSET %dUL'%position(b'MDRV',11),'#define DRIVER_SIZE %dUL'%len(resources[b'MDRV',11])]
    lines+=['typedef struct {const char *name; uint32_t size; const char *hash;} Support;', 'static const Support support_files[]={']
    for directory,manifest,name in [('core','core-inputs','relocations.bin'),('core','core-inputs','file-catalog.bin'),('ui','ui-inputs','ui-seams.bin'),('audio','audio-inputs','audio-pitch.bin')]:
        p=ROOT/'build'/directory/name;h=pins(ROOT/'manifest'/f'{manifest}.sha256')[name]
        assert hashlib.sha256(p.read_bytes()).hexdigest()==h
        lines.append('{%s,%dUL,%s},'%(json.dumps(name),p.stat().st_size,json.dumps(h)))
    lines+=['};']
    return '\n'.join(lines)+'\n'
if __name__=='__main__':
    p=Path(__file__).with_name('support.h');s=generate()
    if '--check' in sys.argv: assert p.read_text()==s
    else:p.write_text(s)
