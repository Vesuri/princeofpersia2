"""Derive pinned audio data from the original archive; never emit original tables into source."""
import hashlib
from extract_original_data import ROOT, DESTINATION, verify_output, pins
import trap_census as census
from driver_image import decrypt, expand

def content():
    verify_output(DESTINATION);census.load(DESTINATION/'Prince of Persia 2.rsrc')
    driver=expand(decrypt(census.R[b'MDRV',11].body))
    return {'driver-expanded.bin':driver,'audio-pitch.bin':driver[0x7256:0x7456]}

def generate(verify=False):
    assets=content();expected=pins(ROOT/'manifest/audio-inputs.sha256')
    for name,data in assets.items():
        if hashlib.sha256(data).hexdigest()!=expected[name]:raise ValueError('AUDIO / INPUT PIN '+name)
    digest=expected['audio-pitch.bin']
    header='// Generated hashes only; original pitch data is loaded separately.\nstatic const uint8_t audioPitchSHA256[32]={'+','.join('0x'+digest[i:i+2] for i in range(0,64,2))+'};\n'
    out=ROOT/'build/audio'
    if verify:
        if any((out/name).read_bytes()!=data for name,data in assets.items()) or (out/'AudioInputs.h').read_text()!=header:raise ValueError('AUDIO / GENERATED INPUT')
    else:
        out.mkdir(parents=True,exist_ok=True)
        for name,data in assets.items():(out/name).write_bytes(data)
        (out/'AudioInputs.h').write_text(header)
    print('AUDIO INPUTS PASS: verified original driver and pitch data')

if __name__=='__main__':
    import sys
    generate('--verify' in sys.argv)
