"""Decode native display-service replay output against pinned Macintosh scenes."""
import argparse,re,struct
from pathlib import Path
from check_core_result import restored
from display_frames import frames


def decode_planes(data):
    assert len(data)==196608,'DISPLAY / PLANE EXTENT'
    result=bytearray(512*384)
    for y in range(384):
        for plane in range(8):
            for x in range(64):
                byte=data[y*512+plane*64+x]
                for bit in range(8):result[y*512+x*8+bit]|=((byte>>(7-bit))&1)<<plane
    return bytes(result)


def decode_copper(code,base,parity):
    assert len(code)==2236 and code[-4:]==b'\xff\xff\xff\xfe','DISPLAY / COPPER EXTENT'
    regs={};colors=[0]*256;bank=0;low=False;writes=set()
    for reg,value in struct.iter_unpack('>HH',code[:-4]):
        assert not reg&1 and reg<=0x1fc,'DISPLAY / COPPER MOVE'
        regs[reg]=value
        if reg==0x106:bank=value>>13;low=bool(value&512)
        if 0x180<=reg<=0x1be:
            i=bank*32+(reg-0x180)//2;writes.add((i,low))
            bits=((value&0xf00)<<8)|((value&0xf0)<<4)|(value&15)
            if low:colors[i]=(colors[i]&0xf0f0f0)|bits
            else:colors[i]=(colors[i]&0x0f0f0f)|(bits<<4)
    assert len(writes)==512,'DISPLAY / COMPLETE PALETTE'
    for p in range(8):
        address=regs[0xe0+4*p]*65536+regs[0xe2+4*p]
        assert address==base+parity*512+p*64,'DISPLAY / FIELD POINTER'
        # Each field consumes 64 bytes per plane, then skips the other seven
        # planes and the opposite field's row. The last fetch stays in storage.
        assert address-base+191*(64+regs[0x108])+64<=196608,'DISPLAY / DMA EXTENT'
    expected={0x1fc:3,0x100:0x8215,0x102:0,0x104:0x24,0x106:0xc60,0x10c:0,
              0x8e:0x4ca1,0x90:0x0ca1,0x1e4:0x2100,0x92:0x48,0x94:0xb8,0x108:960,0x10a:960}
    assert all(regs.get(k)==v for k,v in expected.items()),'DISPLAY / MODE'
    return b''.join(c.to_bytes(3,'big') for c in colors)


def check(run,reference):
    frames(reference) # Verifies pins, checkpoint completion and RGB/index agreement.
    log=(run/'gdb-out.log').read_text();restored(run,log)
    assert 'CORE REASON DISPLAY / FIXTURE COMPLETE\n' in log,'DISPLAY / COMPLETION'
    assert 'CORE EXCEPTION vector=0 pc=00000000\n' in log,'DISPLAY / EXCEPTION'
    rows=re.findall(r'DISPLAY_SCENE id=(\d+) planes=([0-9a-f]+) copper=([0-9a-f]+) parity=(\d+) line=(\d+) fields=(\d+) frames=(\d+) late=(\d+)',log)
    assert len(rows)==2,'DISPLAY / TWO SCENES REQUIRED'
    for n,(label,row) in enumerate(zip(('title','first-room'),rows),1):
        ident,base,copper,parity,line,fields,published,late=row
        assert int(ident)==n and int(parity) in (0,1) and int(line)<16 and int(late)==0 and int(published)>=2*n,'DISPLAY / PUBLICATION'
        original=(reference/(label+'.indices')).read_bytes()
        expected=b''.join(original[y*640+65:y*640+577] for y in range(48,432))
        logical=(run/f'scene-{n}-logical.bin').read_bytes()
        assert len(logical)==307200,'DISPLAY / LOGICAL EXTENT'
        assert b''.join(logical[y*640+65:y*640+577] for y in range(48,432))==expected,'DISPLAY / LOGICAL CLIENT'
        assert decode_planes((run/f'scene-{n}-planes.bin').read_bytes())==expected,'DISPLAY / INDEXED SCENE'
        assert f'DISPLAY_EVEN id={n} lof=1\n' in log and int(parity)==0,'DISPLAY / EVEN LOF'
        odd=re.search(rf'DISPLAY_ODD id={n} planes=([0-9a-f]+) copper=([0-9a-f]+) parity=1 lof=0 fields=(\d+) frames=(\d+) late=0',log)
        assert odd and odd[1]==base and int(odd[3])==int(fields)+1 and int(odd[4])==int(published),'DISPLAY / PAIRED FIELDS'
        assert int(odd[2],16)==int(copper,16)+2236,'DISPLAY / ODD COPPER SELECTION'
        odd_rgb=decode_copper((run/f'scene-{n}-odd-copper.bin').read_bytes(),int(base,16),1)
        rgb=decode_copper((run/f'scene-{n}-copper.bin').read_bytes(),int(base,16),int(parity))
        assert odd_rgb==rgb,'DISPLAY / FIELD PALETTE IDENTITY'
        assert rgb==(reference/(label+'.rgb')).read_bytes(),'DISPLAY / SCENE PALETTE'
    print('DISPLAY SCENES PASS: title and first room service replay; exact client indices, AGA planes and RGB24 copper')

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('run',type=Path);p.add_argument('reference',type=Path);a=p.parse_args();check(a.run,a.reference)
