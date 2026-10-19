"""Inherited no-float/probe discipline, plus dormant-link and HUNK audits."""
import os,re,subprocess,sys
from pathlib import Path
elf,partial,*probes=sys.argv[1:]
def symbols(path):return subprocess.check_output(['m68k-amiga-elf-objdump','-t',path],text=True)
s=symbols(elf)
assert not re.search(r'\b__[a-z0-9_]*(?:sf|df|xf|tf)[a-z0-9_]*$',s,re.M),'software float helper'
for name in probes:
    assert re.search(r'\bO\s+\.(?:bss|data)\s+.*\b'+name+r'$',s,re.M),'missing data probe: '+name
unknown=set(re.findall(r'\*UND\*\s+\S+\s+(\S+)',symbols(partial)))
allowed={'__preinit_array_start','__preinit_array_end','__init_array_start','__init_array_end','__fini_array_start','__fini_array_end'}
assert not unknown-allowed,'dormant unresolved references: '+str(unknown-allowed)
exe=Path(elf).with_suffix('.exe').read_bytes()
assert exe[:4]==b'\0\0\x03\xf3','not HUNK_HEADER'
version=Path('../VERSION').read_text().strip().encode()
assert b'$VER: Prince of Persia 2 '+version+b' ' in exe,'version mismatch'
assert 'unused_shared_start' not in s,'wrong CRT entry retained'
print('LINK AUDIT PASS: HUNK, version, no float, retained data probes, complete partial link')

# Verification must never leak into the release executable. Ignore constructor
# names containing a source-file name; inspect actual function/data symbols.
if os.environ.get('POP2_VERIFY_BUILD')=='0':
    names={line.split()[-1] for line in s.splitlines() if len(line.split())>=6}
    assert not any(re.match(r'_Z[0-9]+sha_(init|update|final)',name) for name in names),'release contains runtime SHA-256'
    assert not names.intersection({'g_coreTrace','g_coreCount','g_fileVerifiedFiles','g_fileVerifiedBytes','g_coreResources','g_coreResourceCount','g_fileVolume','g_fileVolumeQueries','g_displayCopyInterrupts','g_blockMoveChecks','g_quickWarmChecks','g_hunlockChecks','g_hunlockRejected','g_hunlockFreed'}),'release contains verification counters/trace'
    assert not any('BlockMoveChecks' in name for name in names),'release contains exhaustive copy fixtures'
    print(('PROFILE' if os.environ.get('POP2_PROFILE_BUILD')=='1' else 'RELEASE')+' AUDIT PASS: no runtime SHA-256, trap trace or hash counters')
