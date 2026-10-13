"""Require the original-paired session save/load route and persisted document."""
import argparse
import hashlib
import json
import re
import shutil
import tempfile
from pathlib import Path
from check_core_result import restored
from check_gameplay_restart import paired_frame
from check_integration_result import require
from check_display_scenes import decode_copper
from extract_original_data import ROOT, pins


def check(run, reference, setup):
    for name,digest in pins(ROOT/'manifest/gameplay-save-reference.sha256').items():
        require(hashlib.sha256((reference/name).read_bytes()).hexdigest()==digest,'SAVE REFERENCE PIN '+name)
    original=(reference/'session.log').read_text()
    for marker in ('GAMEPLAY_MENU ordinal=1 command=01910005',
                   'GAMEPLAY_MENU ordinal=2 command=01910004',
                   'GAMEPLAY_FILE_COMPLETE save-return','GAMEPLAY_FILE_COMPLETE load-return',
                   'GAMEPLAY_PROTECTION_ACCEPTED decision=D6','GAMEPLAY_LOADED_FRAME result=FFFE',
                   'GAMEPLAY_VOLUME_FLUSHED','GAMEPLAY_REFERENCE_COMPLETE route=save-load loaded_frames=1'):
        require(original.count(marker+'\n')==1,'SAVE ORIGINAL COMPLETION '+marker)
    require('FAIL REFERENCE' not in original,'SAVE ORIGINAL ERROR')
    receipt=json.loads((run/'build-receipt.json').read_text())
    require({'-DPOP2_VERIFY','-DPOP2_GAMEPLAY_SAVE_PROBE','-DPOP2_GAMEPLAY_RESTART_PROBE','-DPOP2_INTEGRATION_PROBE'}.issubset(receipt['flags'].split()),'SAVE BUILD FLAGS')
    for key,name in (('exe','dh1/PrinceOfPersia2'),('elf','PrinceOfPersia2.elf')):
        require(hashlib.sha256((run/name).read_bytes()).hexdigest()==receipt[key],'SAVE BUILD IDENTITY')
    log=(run/'gdb-out.log').read_text();restored(run,log,setup)
    require(log.count('CORE REASON GAMEPLAY / SAVE LOAD COMPLETE\n')==1,'SAVE COMPLETION')
    require('INTEGRATION AUDIT code=1 input=1\n' in log,'SAVE CODE/INPUT AUDIT')
    require(re.findall(r'^INTEGRATION STAGE (\d+) ',log,re.M)==['1','2','3','5','4'],'SAVE STAGE ORDER')
    require(re.findall(r'^INTEGRATION ALERT id=(\d+)$',log,re.M)==['7001','7002'],'SAVE PICKER ORDER')
    require(re.findall(r'^GAMEPLAY FILE save=(\d+) load=(\d+)$',log,re.M)==[('1','0'),('1','1')],'SAVE ORIGINAL ROUTINE RETURNS')
    require(re.findall(r'^GAMEPLAY AUDIO selector=\d+ menu=([0-9a-f]+)$',log,re.M)==['01910004'],'SAVE OPEN COMMAND')
    require('GAMEPLAY INPUT actions=7 ' in log,'SAVE INPUT ACTIONS')
    pattern=r' step=(\d+) left=(\d+) right=(\d+) command=(\d+)$'
    original_keys=re.findall(r'^GAMEPLAY_KEYS'+pattern,original,re.M)
    native_keys=re.findall(r'^GAMEPLAY KEYS'+pattern,log,re.M)
    require(len(original_keys)>20,'SAVE ORIGINAL KEY TRACE')
    require(native_keys==original_keys,'SAVE MATCHED INPUT STEPS')
    clocks=re.findall(r'^GAMEPLAY SUSPEND calls=2/2 clock=(\d+)/(\d+)$',log,re.M)
    require(len(clocks)==1 and clocks[0][0]==clocks[0][1],'SAVE AUDIO SUSPEND/RESUME')
    pairs=(('first-room','gameplay-first-frame-1'),('loaded-first','gameplay-first-frame-2'),('loaded','gameplay-loaded-frame'))
    for label,prefix in pairs:
        paired_frame(run,reference,label,prefix)
    # DERIVED: CODE 4+3272 copies the 62-byte kid record at A5-50D2
    # into the active character record; no pointers or clock masks are used.
    for label,prefix in pairs+(('saved','gameplay-save-return'),('load-return','gameplay-load-return')):
        native=(run/(label+'-a5.bin')).read_bytes()
        mac=(reference/(prefix+'-a5.bin')).read_bytes()
        require(native[-0x50d2:-0x5094]==mac[-0x50d2:-0x5094],label+' PLAYER RECORD')
    for root,saved,moved in ((run,'saved','load-return'),(reference,'gameplay-save-return','gameplay-load-enter')):
        before=(root/(saved+'-a5.bin')).read_bytes()[-0x50d2:-0x5094]
        after=(root/(moved+'-a5.bin')).read_bytes()[-0x50d2:-0x5094]
        require(before!=after,'SAVE MOVEMENT MUST CHANGE PLAYER RECORD')
    for label,pattern,prefix in (('loaded-first','GAMEPLAY FIRST DISPLAY','gameplay-first-frame-2'),('loaded','GAMEPLAY DISPLAY','gameplay-loaded-frame')):
        fields=re.findall(r'^'+pattern+r' planes=([0-9a-f]+) parity=(\d+)$',log,re.M)
        require(len(fields)==1,'SAVE COPPER RECORD '+label)
        require(decode_copper((run/(label+'-copper.bin')).read_bytes(),int(fields[0][0],16),int(fields[0][1]))==(reference/(prefix+'.rgb')).read_bytes(),label+' COPPER')
    saved=(run/'dh1/Level 1').read_bytes()
    require(saved==(reference/'saved-Level-1.data').read_bytes(),'SAVE PERSISTED DOCUMENT')
    require((run/'dh1/Level 1.rsrc').read_bytes()==(reference/'saved-Level-1.rsrc').read_bytes(),'SAVE RESOURCE FORK')
    metadata=(run/'dh1/Level 1.finfo').read_bytes()
    require(len(metadata)==32 and metadata[:4]==b'AFI1','SAVE METADATA FORMAT')
    checksum=2166136261
    for byte in metadata[:28]:checksum=((checksum^byte)*16777619)&0xffffffff
    require(int.from_bytes(metadata[28:],'big')==checksum,'SAVE METADATA CHECKSUM')
    require(metadata[4:12]==(reference/'saved-Level-1.type-creator').read_bytes(),'SAVE TYPE/CREATOR')
    print('GAMEPLAY SAVE LOAD PASS: original commands, persisted document, movement, paired resumed frames and restoration')


def controls(run, reference, setup):
    with tempfile.TemporaryDirectory(prefix='save-controls-',dir=ROOT/'tmp') as folder:
        target=Path(folder)
        names=['gdb-out.log','build-receipt.json','PrinceOfPersia2.elf','logs/debug.uae',
               'dh1/Shell.done','dh1/PrinceOfPersia2','dh1/video-transfer.bin',
               'dh1/Level 1','dh1/Level 1.rsrc','dh1/Level 1.finfo',
               'loaded-first-copper.bin','loaded-copper.bin','saved-a5.bin','load-return-a5.bin']
        for label in ('first-room','loaded-first','loaded'):
            names += [label+'-'+suffix+'.bin' for suffix in ('screen','planes','colors','context','a5')]
        for name in names:
            destination=target/name;destination.parent.mkdir(parents=True,exist_ok=True)
            shutil.copyfile(run/name,destination)
        check(target,reference,setup)
        def flip(data,at):return data[:at]+bytes([data[at]^1])+data[at+1:]
        mutations=[
            ('gdb-out.log',lambda b:b.replace(b'GAMEPLAY / SAVE LOAD COMPLETE',b'GAMEPLAY / INCOMPLETE')),
            ('gdb-out.log',lambda b:b.replace(b'FILE save=1 load=1',b'FILE save=1 load=0')),
            ('gdb-out.log',lambda b:b.replace(b'actions=7',b'actions=3')),
            ('gdb-out.log',lambda b:b.replace(b'step=14 left=8',b'step=14 left=0')),
            ('gdb-out.log',lambda b:b.replace(b'menu=01910004',b'menu=00000000')),
            ('gdb-out.log',lambda b:b.replace(b'calls=2/2',b'calls=1/1')),
            ('gdb-out.log',lambda b:b.replace(b'heap=1',b'heap=0')),
            ('gdb-out.log',lambda b:b.replace(b'code=1 input=1',b'code=0 input=1')),
            ('logs/debug.uae',lambda b:b.replace(b'cpu_model=68030',b'cpu_model=68040')),
            ('build-receipt.json',lambda b:b.replace(b'POP2_GAMEPLAY_SAVE_PROBE',b'POP2_OTHER_PROBE')),
            ('dh1/PrinceOfPersia2',lambda b:b[:-1]),
            ('dh1/Shell.done',lambda b:b'SHELL PASS\n'),
            ('dh1/Level 1',lambda b:flip(b,0)),
            ('dh1/Level 1.rsrc',lambda b:b+b'X'),
            ('dh1/Level 1.finfo',lambda b:flip(b,31)),
            ('loaded-screen.bin',lambda b:flip(b,48*640+65)),
            ('loaded-planes.bin',lambda b:flip(b,0)),
            ('loaded-colors.bin',lambda b:flip(b,10)),
            ('loaded-context.bin',lambda b:flip(b,0x218a)),
            ('loaded-a5.bin',lambda b:flip(b,len(b)-0x18fc)),
            ('saved-a5.bin',lambda b:flip(b,len(b)-0x50d2)),
            ('loaded-copper.bin',lambda b:b[:-2]),
        ]
        for name,mutate in mutations:
            path=target/name;good=path.read_bytes();bad=mutate(good)
            require(bad!=good,'SAVE CONTROL DID NOT CHANGE EVIDENCE')
            path.write_bytes(bad)
            try:check(target,reference,setup)
            except (RuntimeError,ValueError,AssertionError):pass
            else:raise RuntimeError('SAVE / ACCEPTED ALTERED EVIDENCE '+name)
            finally:path.write_bytes(good)
        print('GAMEPLAY SAVE CONTROLS PASS:',len(mutations))
        return len(mutations)


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run',type=Path);parser.add_argument('reference',type=Path)
    parser.add_argument('--setup',required=True)
    parser.add_argument('--controls',action='store_true')
    args=parser.parse_args();check(args.run,args.reference,args.setup)
    if args.controls:controls(args.run,args.reference,args.setup)
