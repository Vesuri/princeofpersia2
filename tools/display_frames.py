"""Verify the two Macintosh scenes and prepare a local native presentation fixture."""
import hashlib,struct,sys
from pathlib import Path
from extract_original_data import ROOT,pins

def frames(reference):
    log=(reference/'session.log').read_text()
    assert 'FAIL REFERENCE' not in log and '[LUA ERROR]' not in log
    result=bytearray(b'DFR1'+struct.pack('>I',2))
    lookup=(ROOT/'build/display/video-transfer.bin').read_bytes()
    assert hashlib.sha256(lookup).hexdigest()==pins(ROOT/'manifest/display-inputs.sha256')['video-transfer.bin']
    expected=pins(ROOT/'manifest/reference-display.sha256')
    for label in ('title','first-room'):
        assert log.count('DISPLAY_ROUTE_CAPTURED '+label+'\n')==1
        data={suffix:(reference/(label+'.'+suffix)).read_bytes() for suffix in ('indices','rgb','clut','ppm')}
        for suffix,body in data.items():
            assert hashlib.sha256(body).hexdigest()==expected[label+'.'+suffix],'DISPLAY / REFERENCE PIN '+label+'.'+suffix
        indices,rgb,clut=data['indices'],data['rgb'],data['clut']
        assert len(indices)==640*480 and len(rgb)==768 and len(clut)==2056
        assert data['ppm']==b'P6\n640 480\n255\n'+b''.join(rgb[i*3:i*3+3] for i in indices),'DISPLAY / RGB AND INDEX AGREEMENT'
        assert bytes(lookup[clut[8+8*i+2+2*c]] for i in range(256) for c in range(3))==rgb,'DISPLAY / MEASURED COLOUR TRANSFER'
        result.extend(indices+clut+rgb)
    return bytes(result)

def generate(reference):
    blob=frames(reference);digest=hashlib.sha256(blob).hexdigest()
    assert digest==pins(ROOT/'manifest/display-inputs.sha256')['display-frames.bin'],'DISPLAY / FIXTURE PIN'
    out=ROOT/'build/display';out.mkdir(parents=True,exist_ok=True)
    (out/'display-frames.bin').write_bytes(blob)
    (out/'DisplayFrames.h').write_text('static const uint8_t displayFramesSHA256[32]={'+','.join('0x'+digest[i:i+2] for i in range(0,64,2))+'};\n')
    print('DISPLAY INPUT PASS: title and first room; indexed/RGB/CLUT agreement;',digest)
if __name__=='__main__':generate(Path(sys.argv[1]))
