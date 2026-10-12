"""Stage only verified local core inputs into a fresh private emulator volume."""
import os,shutil,sys
from pathlib import Path
from extract_original_data import ROOT,DESTINATION,verify_output
verify_output(DESTINATION)
# Runtime independently checks the application and relocation-plan digests.
for source,name in [(DESTINATION/'Prince of Persia 2.rsrc','Prince of Persia 2.rsrc'),
                    (ROOT/'build/core/relocations.bin','relocations.bin')]:
    target=Path(sys.argv[1])/name
    if target.exists():raise RuntimeError('CORE / PRIVATE INPUT ALREADY EXISTS')
    shutil.copyfile(source,target)

control=os.environ.get('CORE_CONTROL')
if control:
    names={'bad-application':'Prince of Persia 2.rsrc','bad-plan':'relocations.bin'}
    if control not in names:raise RuntimeError('CORE / UNKNOWN INPUT CONTROL')
    target=Path(sys.argv[1])/names[control]
    data=bytearray(target.read_bytes());data[0]^=1;target.write_bytes(data)
