"""Prepare the pinned local video transfer without tracking original-derived bytes."""
import hashlib,os
from extract_original_data import ROOT,pins

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
    digest=hashlib.sha256(path.read_bytes()).hexdigest()
    assert len(path.read_bytes())==256 and digest==pins(ROOT/'manifest/display-inputs.sha256')['video-transfer.bin'],'DISPLAY / VIDEO INPUT PIN'
    (path.parent/'VideoTransfer.h').write_text('static const uint8_t videoTransferSHA256[32]={'+','.join('0x'+digest[i:i+2] for i in range(0,64,2))+'};\n')
    print('DISPLAY INPUT VERIFIED:',digest)
if __name__=='__main__':prepare()
