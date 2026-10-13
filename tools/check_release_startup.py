"""Verify unmodified release startup at the original first-text checkpoint."""
import hashlib
import subprocess
import sys
from pathlib import Path
from extract_original_data import ROOT,pins


def check(run,reference):
    log=(run/'gdb-out.log').read_text()
    assert log.count('RELEASE OPENING COMPLETE segment=15 offset=0416 text=6978 page=1\n')==1,'RELEASE / POSITIVE COMPLETION'
    assert 'RELEASE STARTUP FAILED' not in log,'RELEASE / EARLY STOP'
    symbols=subprocess.check_output(['m68k-amiga-elf-objdump','-t',str(run/'PrinceOfPersia2.elf')],text=True)
    names={line.split()[-1] for line in symbols.splitlines() if len(line.split())>=6}
    assert not any(name.startswith(('_Z8sha_init','_Z10sha_update','_Z9sha_final')) for name in names),'RELEASE / HASH CODE'
    assert not names.intersection({'g_coreTrace','g_coreCount','g_fileVerifiedFiles','g_fileVerifiedBytes','g_coreResources','g_coreResourceCount','g_fileVolume','g_fileVolumeQueries','g_integrationStage'}),'RELEASE / VERIFICATION OR INPUT PROBES'
    original=(reference/'opening.ppm').read_bytes()
    assert hashlib.sha256(original).hexdigest()==pins(ROOT/'manifest/reference-results.sha256')['opening.ppm'],'RELEASE / ORIGINAL PIN'
    header=b'P6\n640 480\n255\n'
    assert original.startswith(header) and len(original)==len(header)+640*480*3
    indices=(run/'release-screen.bin').read_bytes();colors=(run/'release-colors.bin').read_bytes()
    assert len(indices)==640*480 and len(colors)==2056,'RELEASE / CAPTURE EXTENT'
    transfer=(ROOT/'build/display/video-transfer.bin').read_bytes()
    assert hashlib.sha256(transfer).hexdigest()==pins(ROOT/'manifest/display-inputs.sha256')['video-transfer.bin']
    palette=[bytes(transfer[colors[8+8*i+2+2*c]] for c in range(3)) for i in range(256)]
    rgb=b''.join(palette[i] for i in indices)
    def viewport(data):
        return b''.join(data[(y*640+65)*3:(y*640+577)*3] for y in range(48,432))
    assert viewport(rgb)==viewport(original[len(header):]),'RELEASE / ORIGINAL OPENING PIXELS'
    print('RELEASE STARTUP PASS: original first-text checkpoint and full viewport pixels; no runtime hashing or trap trace')


if __name__=='__main__':
    check(Path(sys.argv[1]),Path(sys.argv[2]))
