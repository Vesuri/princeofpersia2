"""Decode captured native planes/copper independently of the production converter."""
import argparse,re,struct
from pathlib import Path
from scaffold_result import check as check_restored

def copy_completion(log):
    copies=re.findall(r'^DISPLAY_COPY interrupts=(\d+)$',log,re.M)
    assert len(copies)==1 and int(copies[0])>0,'DISPLAY / ASYNCHRONOUS COPY COMPLETION'

def check(run,setup="a1200-fast",controls=False):
    check_restored(run,setup)
    log=(run/'gdb-out.log').read_text()
    copy_completion(log)
    if controls:
        for replacement in ('','DISPLAY_COPY interrupts=0\n'):
            bad=re.sub(r'^DISPLAY_COPY interrupts=\d+\n',replacement,log,flags=re.M)
            assert bad!=log,'DISPLAY / INACTIVE COMPLETION CONTROL'
            try:copy_completion(bad)
            except AssertionError:pass
            else:raise AssertionError('DISPLAY / ACCEPTED MISSING COMPLETION')
        print('DISPLAY COPY CONTROLS PASS: missing and zero completion rejected')
    rows=re.findall(r'DISPLAY_FRAME id=(\d+) planes=([0-9a-f]+) copper=([0-9a-f]+) parity=(\d+) line=(\d+) fields=(\d+) frames=(\d+) late=(\d+)',log)
    assert len(rows)==4,'DISPLAY / FOUR COMPLETED FRAMES REQUIRED'
    chunky=bytearray(((x*3+y*7)^(x>>3))&255 for y in range(480) for x in range(640))
    rgb=[(i<<16)|((255-i)<<8)|(i^0x55) for i in range(256)]
    for n,row in enumerate(rows,1):
        ident,base,copper,parity,line,fields,frames,late=row
        assert int(ident)==int(frames)==n and int(line)<16 and int(late)==0,'DISPLAY / PUBLICATION'
        if n==2:
            for y in range(101,139):
                for x in range(123,210):chunky[y*640+x]^=0x6d
        if n==3:rgb=[v^0xa537c1 for v in rgb]
        left,top=(65,47) if n==4 else (64,48)
        data=(run/f'display-{n}-planes.bin').read_bytes()
        assert len(data)==196608,'DISPLAY / PLANE EXTENT'
        for y in range(384):
            for x in range(512):
                pixel=sum(((data[y*512+b*64+x//8]>>(7-x%8))&1)<<b for b in range(8))
                assert pixel==chunky[(y+top)*640+x+left],f'DISPLAY / PIXEL {n} {x},{y}'
        code=(run/f'display-{n}-copper.bin').read_bytes()
        assert len(code)==2236 and code[-4:]==b'\xff\xff\xff\xfe','DISPLAY / COPPER EXTENT'
        regs={};colors=[0]*256;bank=0;lo=False
        for reg,value in struct.iter_unpack('>HH',code[:-4]):
            assert not reg&1 and reg<=0x1fc,'DISPLAY / COPPER MOVE'
            regs[reg]=value
            if reg==0x106:bank=value>>13;lo=bool(value&512)
            if 0x180<=reg<=0x1be:
                i=bank*32+(reg-0x180)//2
                bits=((value&0xf00)<<8)|((value&0xf0)<<4)|(value&15)
                if lo:colors[i]=(colors[i]&0xf0f0f0)|bits
                else:colors[i]=(colors[i]&0x0f0f0f)|(bits<<4)
        assert colors==rgb,'DISPLAY / RGB24 PALETTE'
        for p in range(8):
            address=regs[0xe0+4*p]*65536+regs[0xe2+4*p]
            assert address==int(base,16)+int(parity)*512+p*64,'DISPLAY / FIELD POINTER'
        expected={0x1fc:3,0x100:0x8215,0x102:0,0x104:0x24,0x106:0xc60,0x10c:0,
                  0x8e:0x4ca1,0x90:0x0ca1,0x1e4:0x2100,0x92:0x48,0x94:0xb8,0x108:960,0x10a:960}
        assert all(regs[k]==v for k,v in expected.items()),'DISPLAY / MODE'
    print('DISPLAY PROBE PASS: four full decoded frames, dirty preservation, RGB24, viewport and field pointers')
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('run',type=Path)
    p.add_argument('--setup',default='a1200-fast',choices=['a1200-fast','a1200-030','a1200-040'])
    p.add_argument('--controls',action='store_true')
    a=p.parse_args();check(a.run,a.setup,a.controls)
