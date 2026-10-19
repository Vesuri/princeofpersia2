"""Prepare the pinned local video transfer without tracking original-derived bytes."""
import hashlib,os,sys
from extract_original_data import ROOT,pins

def header(table):
    digest=hashlib.sha256(table).hexdigest()
    assert len(table)==256 and digest==pins(ROOT/'manifest/display-inputs.sha256')['video-transfer.bin'],'DISPLAY / VIDEO INPUT PIN'
    return 'static const uint8_t videoTransfer[256]={'+','.join(str(v) for v in table)+'};\n'

def prepare():
    path=ROOT/'build/display/video-transfer.bin'
    if not path.exists():
        from run_reference import run
        from video_transfer import generate
        saved=dict(os.environ)
        try:
            for key in list(os.environ):
                if key.startswith('POP2_'):os.environ.pop(key)
            os.environ['POP2_VIDEO_TRANSFER']='1';generate(run())
        finally:os.environ.clear();os.environ.update(saved)
    table=path.read_bytes();content=header(table)
    target=path.parent/'VideoTransfer.h'
    if '--verify' in sys.argv:
        assert target.read_text()==content,'DISPLAY / EMBEDDED TABLE HEADER'
    elif not target.exists() or target.read_text()!=content:
        target.write_text(content)
    print('DISPLAY INPUT VERIFIED:',hashlib.sha256(table).hexdigest())
if __name__=='__main__':prepare()
