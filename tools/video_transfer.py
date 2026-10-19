"""Verify the Macintosh channel sweep and emit its local, pinned transfer table."""
import hashlib,re,struct,sys
from pathlib import Path
from extract_original_data import ROOT,pins

def measure(reference):
    log=(reference/'session.log').read_text()
    assert 'FAIL REFERENCE' not in log and '[LUA ERROR]' not in log
    assert log.count('VIDEO_TRANSFER_COMPLETE values=65536 mixed=256 calls=257 stack=8')==1
    assert [int(n) for n in re.findall(r'^VIDEO_BATCH batch=(\d+) ',log,re.M)]==list(range(257))
    inputs=(reference/'video-input.bin').read_bytes();outputs=(reference/'video-output.bin').read_bytes()
    assert len(inputs)==257*256*8 and len(outputs)==257*256*4
    table={}
    for batch in range(257):
        for i in range(256):
            at=batch*256+i
            index,r,g,b=struct.unpack_from('>4H',inputs,at*8)
            assert index==i
            expected=(i*256+batch,)*3 if batch<256 else (i*257,(255-i)*257,((i*71)%256)*257)
            assert (r,g,b)==expected,'VIDEO / ACTUAL CALL INPUT'
            value=struct.unpack_from('>I',outputs,at*4)[0]
            assert value>>24==255
            for source,target in zip((r,g,b),(value>>16&255,value>>8&255,value&255)):
                key=source>>8
                assert key not in table or table[key]==target,'VIDEO / LOW BYTE OR CHANNEL DEPENDENCE'
                table[key]=target
    assert set(table)==set(range(256))
    return bytes(table[i] for i in range(256))

def generate(reference):
    table=measure(reference);digest=hashlib.sha256(table).hexdigest()
    expected=pins(ROOT/'manifest/display-inputs.sha256')['video-transfer.bin']
    assert digest==expected,'VIDEO / TABLE PIN'
    out=ROOT/'build/display';out.mkdir(parents=True,exist_ok=True)
    (out/'video-transfer.bin').write_bytes(table)
    # Generated measured bytes stay ignored, but are linked into the executable.
    from display_inputs import header
    (out/'VideoTransfer.h').write_text(header(table))
    print('VIDEO TRANSFER PASS: all 65536 values and 256 mixed colours;',digest)
if __name__=='__main__':generate(Path(sys.argv[1]))
