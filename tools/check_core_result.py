"""Pair native initialization with the pinned original; missing evidence fails."""
import hashlib,os,re,struct,sys
from pathlib import Path
from extract_original_data import ROOT,DESTINATION,pins,verify_output
from report_reference import analyze
from static_map import a5world
from amigaxdev.mac.resource_fork import read_resource_fork

def require(ok,why):
    if not ok:raise RuntimeError('CORE / '+why)
def match(pattern,text):
    result=re.search(pattern,text,re.M);require(result is not None,'MISSING '+pattern);return result

def restored(run,log):
    m=match(r'^CORE RESTORE stage=3 error=0 vector=1 heap=1 zone=3145728 dma=(\d+)/(\d+) irq=(\d+)/(\d+) view=1 vbi=1$',log)
    require(m[1]==m[2] and m[3]==m[4],'RESTORED HARDWARE')
    require((run/'dh1/Shell.done').read_text().strip()=='SHELL FAIL','NAMED STOP MUST RETURN DOS FAIL')
    config=dict(line.split('=',1) for line in (run/'logs/debug.uae').read_text().splitlines() if '=' in line)
    expected={'cpu_model':'68020','chipset':'aga','chipmem_size':'4','fastmem_size':'8','ntsc':'false','cpu_speed':'real','cachesize':'0','cpu_frequency':'14187580'}
    require(all(config.get(k)==v for k,v in expected.items()),'EFFECTIVE MACHINE')

def initialized(resources,directory,a5,code1,handles):
    initial=a5world(resources,a5)[0]
    require((directory/'a5-initial.bin').read_bytes()==initial,'ORIGINAL DATA/ZERO/DREL INITIALIZATION')
    expected=bytearray(initial);below=len(expected)
    # Measured constructor writes, expressed in original A5-relative coordinates.
    for off,value in [(-0x1950,handles[0x5f535449]),(-0x194c,handles[0x5f535444]),
                      (-0x1856,a5+0x13ba),(-0x1796,code1+0x404)]:
        struct.pack_into('>I',expected,below+off,value)
    struct.pack_into('>H',expected,below-0x1868,3)
    require((directory/'a5-core.bin').read_bytes()==expected,'COMPLETE A5 AFTER ORIGINAL CONSTRUCTORS')

def check(run,reference=None):
    reference=Path(reference or os.environ['CORE_REFERENCE'])
    if not reference.is_absolute():reference=ROOT/reference
    log=(run/'gdb-out.log').read_text();rlog=(reference/'session.log').read_text()
    require('CORE STOP trap=a86e segment=2 offset=3e90 count=30' in log and 'CORE REASON UNIMPLEMENTED / InitGraf' in log,'INITIALIZATION STOP')
    restored(run,log)
    trace,facts=analyze(rlog);pin=pins(ROOT/'manifest/reference-results.sha256')
    require(hashlib.sha256(trace).hexdigest()==pin['traps.txt'],'REFERENCE TRAP PIN')
    require(hashlib.sha256((reference/'opening.ppm').read_bytes()).hexdigest()==pin['opening.ppm'],'REFERENCE COMPLETION PIN')
    prefix=trace.splitlines(keepends=True)
    boundary=b'CODE 02+3E90 trap=A86E\n';require(boundary in prefix,'REFERENCE InitGraf')
    prefix=b''.join(prefix[:prefix.index(boundary)+1])
    native=''.join(re.findall(r'^CODE \d+\+[0-9A-F]+ trap=[0-9A-F]+\n',log,re.M)).encode()
    require(native==prefix,'ORDERED ORIGINAL TRAP PREFIX')
    require(hashlib.sha256(prefix).hexdigest()==pins(ROOT/'manifest/core-inputs.sha256')['startup-traps.txt'],'STARTUP TRACE PIN')
    identity={bytes.fromhex(s).decode('mac_roman'):{'error':int(e,16),'value':int(v,16)}
              for s,e,v in re.findall(r'^CORE IDENTITY selector=([0-9A-F]+) error=([0-9A-F]+) value=([0-9A-F]+)$',log,re.M)}
    require(identity==facts['gestalt'],'NATIVE GESTALT VS ORIGINAL')
    require(match(r'^CORE SYSENV error=0 data=([0-9A-F]+)$',log)[1]==facts['sysenvirons'].replace('/',''),'NATIVE SYSENV VS ORIGINAL')
    verify_output(DESTINATION)
    resources=read_resource_fork(DESTINATION/'Prince of Persia 2.rsrc')
    code={r.rid:r.body for r in resources if r.kind==b'CODE'}
    na5=int(match(r'^CORE A5 ([0-9a-f]+) below=22266 initial=',log)[1],16)
    ra5=int(match(r'^CORE_INITIALIZED a5=([0-9A-F]+) pc=',rlog)[1],16)
    native_segments={int(i):(int(b,16),int(n)) for i,b,n in re.findall(r'^CORE SEGMENT id=(\d+) base=([0-9a-f]+) size=(\d+)$',log,re.M)}
    ref_segments={int(i):int(b,16) for i,b in re.findall(r'^MAP seg=(\d+) base=([0-9A-F]+) size=\d+$',rlog,re.M)}
    handles={int(t,16):int(h,16) for t,h in re.findall(r'^CORE RESOURCE type=([0-9A-F]+) id=0 handle=([0-9a-f]+)$',log,re.M)}
    rhandles={int(t,16):int(h,16) for t,h in re.findall(r'^CORE_RESOURCE type=([0-9A-F]+) handle=([0-9A-F]+)$',rlog,re.M)}
    initialized(resources,reference,ra5,ref_segments[1],rhandles)
    initialized(resources,run,na5,native_segments[1][0],handles)
    plan=(run/'dh1/relocations.bin').read_bytes()
    require(hashlib.sha256(plan).hexdigest()==pins(ROOT/'manifest/core-inputs.sha256')['relocations.bin'],'RELOCATION PLAN PIN')
    require(set(native_segments)=={1,2,19,21,22},'LOADED SEGMENTS')
    for seg,(base,size) in native_segments.items():
        expected=bytearray(code[seg]);require(size==len(expected),'SEGMENT SIZE')
        for at in range(4,len(plan),32):
            sid,n,offset=struct.unpack_from('>HHI',plan,at)
            if sid==seg:
                require(expected[offset:offset+n]==plan[at+8:at+8+n],'RELOCATION ORIGINAL BYTES')
                expected[offset:offset+n]=plan[at+20:at+20+n]
        if seg==1:struct.pack_into('>I',expected,4,int(match(r'^CORE STRS ([0-9a-f]+)$',log)[1],16))
        if seg==19:struct.pack_into('>I',expected,0x478,na5)
        require((run/f'CODE_{seg}.bin').read_bytes()==expected,'CODE WRITES '+str(seg))
    stub=bytearray((reference/'runtime-stubs.bin').read_bytes())
    require(len(stub)==24,'ORIGINAL STUB EXTENT')
    require(struct.unpack_from('>I',stub,2)[0]==ref_segments[1]+0x64 and struct.unpack_from('>I',stub,14)[0]==ref_segments[1]+0x4c,'ORIGINAL STUB TARGETS')
    m=match(r'^CORE STUB ([0-9a-f]+) oldload=([0-9a-f]+) oldexit=([0-9a-f]+)$',log)
    for off,value in [(2,native_segments[1][0]+0x64),(8,int(m[2],16)),(14,native_segments[1][0]+0x4c),(20,int(m[3],16))]:struct.pack_into('>I',stub,off,value)
    require((run/'runtime-stubs.bin').read_bytes()==stub,'ORIGINAL GENERATED TRAP PATCHES')
    above,below,jump_size,jump_offset=struct.unpack_from('>IIII',code[0])
    expected=bytearray(above);expected[jump_offset:jump_offset+jump_size]=code[0][16:]
    for seg,(base,size) in native_segments.items():
        first,count=struct.unpack_from('>HH',code[seg])
        for i in range(count):
            at=jump_offset+first+8*i;offset=struct.unpack_from('>H',expected,at)[0]
            struct.pack_into('>HHI',expected,at,seg,0x4ef9,base+4+offset)
    struct.pack_into('>I',expected,0x68,int(m[1],16))
    struct.pack_into('>I',expected,0x6c,na5+0x13f2)
    require((run/'a5-above.bin').read_bytes()==expected,'COMPLETE NEAR JUMP TABLE')
    # The Mac launcher's first 32 reserved bytes are not used on this path.
    # Compare every original jump entry, including the runtime's reused slots.
    rexpected=bytearray(code[0][16:]);rbase=int(match(r'^WATCH STUB base=([0-9A-F]+) size=24$',rlog)[1],16)
    for seg in native_segments:
        first,count=struct.unpack_from('>HH',code[seg])
        for i in range(count):
            at=first+8*i;offset=struct.unpack_from('>H',rexpected,at)[0]
            struct.pack_into('>HHI',rexpected,at,seg,0x4ef9,ref_segments[seg]+4+offset)
    struct.pack_into('>II',rexpected,0x68-jump_offset,rbase,ra5+0x13f2)
    require((reference/'a5-above.bin').read_bytes()[jump_offset:]==rexpected,'ORIGINAL NEAR JUMP TABLE')
    print('CORE RUN PASS: original trap order, full A5 state, code writes, generated stubs, native identity, heap and OS restoration')
    return hashlib.sha256(prefix).hexdigest()
if __name__=='__main__':check(Path(sys.argv[1]))
