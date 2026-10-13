"""Require an original-paired restart, complete frame and restored hardware."""
import argparse
import hashlib
import json
import re
import shutil
import tempfile
from pathlib import Path
from check_core_result import restored
from extract_original_data import ROOT, pins
from check_integration_result import crop, require
from check_display_scenes import decode_planes, decode_copper


def paired_frame(run, reference, label, prefix):
    transfer=(run/'dh1/video-transfer.bin').read_bytes()
    require(hashlib.sha256(transfer).hexdigest()==pins(ROOT/'manifest/display-inputs.sha256')['video-transfer.bin'],'VIDEO TRANSFER PIN')
    require(crop((run/(label+'-screen.bin')).read_bytes())==
            crop((reference/(prefix+'.indices')).read_bytes()),label+' PIXELS')
    require(decode_planes((run/(label+'-planes.bin')).read_bytes())==crop((reference/(prefix+'.indices')).read_bytes()),label+' AGA PLANES')
    colors=(run/(label+'-colors.bin')).read_bytes()
    require(len(colors)==2056,label+' PALETTE EXTENT')
    rgb=bytearray(768)
    for i in range(256):
        entry=colors[8+8*i:16+8*i]
        index=i if int.from_bytes(colors[4:6],'big')&0x8000 else int.from_bytes(entry[:2],'big')
        require(index<256,label+' PALETTE INDEX')
        rgb[3*index:3*index+3]=bytes(transfer[entry[j]] for j in (2,4,6))
    require(rgb==(reference/(prefix+'.rgb')).read_bytes(),label+' RGB PALETTE')
    native=(run/(label+'-context.bin')).read_bytes()
    mac=(reference/(prefix+'-context.bin')).read_bytes()
    require(len(native)==len(mac)==0x2200,label+' CONTEXT EXTENT')
    require(native[0x2180:0x21ba]==mac[0x2180:0x21ba],label+' CONTEXT WORDS')
    native=(run/(label+'-a5.bin')).read_bytes()
    mac=(reference/(prefix+'-a5.bin')).read_bytes()
    require(len(native)==len(mac)==22266,label+' A5 EXTENT')
    require(native[-0x18fc:-0x18f8]==mac[-0x18fc:-0x18f8],label+' RNG')


def check(run, reference, setup):
    for name,digest in pins(ROOT/'manifest/gameplay-restart-reference.sha256').items():
        require(hashlib.sha256((reference/name).read_bytes()).hexdigest()==digest,'RESTART REFERENCE PIN '+name)
    original=(reference/'session.log').read_text()
    require('FAIL REFERENCE' not in original, 'RESTART ORIGINAL ERROR')
    require(original.count('GAMEPLAY_REFERENCE_COMPLETE route=restart first_frames=2\n')==1,
            'RESTART ORIGINAL COMPLETION')
    require(original.count('GAMEPLAY_MENU ordinal=1 command=01930009\n')==1,
            'RESTART ORIGINAL COMMAND')
    receipt=json.loads((run/'build-receipt.json').read_text())
    require({'-DPOP2_VERIFY','-DPOP2_GAMEPLAY_RESTART_PROBE','-DPOP2_INTEGRATION_PROBE'}.issubset(receipt['flags'].split()),'RESTART BUILD FLAGS')
    for key,name in (('exe','dh1/PrinceOfPersia2'),('elf','PrinceOfPersia2.elf')):
        require(hashlib.sha256((run/name).read_bytes()).hexdigest()==receipt[key],'RESTART BUILD IDENTITY')
    transfer=(run/'dh1/video-transfer.bin').read_bytes()
    require(hashlib.sha256(transfer).hexdigest()==pins(ROOT/'manifest/display-inputs.sha256')['video-transfer.bin'],'RESTART VIDEO TRANSFER PIN')
    log=(run/'gdb-out.log').read_text()
    restored(run,log,setup)
    require('CORE REASON GAMEPLAY / RESTART COMPLETE\n' in log,'RESTART COMPLETION')
    require('GAMEPLAY RESTART rooms=2\n' in log,'RESTART TWO FIRST FRAMES')
    require('INTEGRATION AUDIT code=1 input=1\n' in log,'RESTART CODE/INPUT AUDIT')
    require(re.findall(r'^INTEGRATION STAGE (\d+) ',log,re.M)==['1','2','3','4'],'RESTART STAGE ORDER')
    require(re.findall(r'^GAMEPLAY AUDIO selector=\d+ menu=([0-9a-f]+)$',log,re.M)==['01930009'],'RESTART NATIVE COMMAND')
    clocks=re.findall(r'^GAMEPLAY SUSPEND calls=1/1 clock=(\d+)/(\d+)$',log,re.M)
    require(len(clocks)==1 and clocks[0][0]==clocks[0][1],'RESTART SUSPENDED CLOCK')
    for ordinal,label in ((1,'first-room'),(2,'restart')):
        paired_frame(run, reference, label, 'gameplay-first-frame-'+str(ordinal))
    fields=re.findall(r'^GAMEPLAY DISPLAY planes=([0-9a-f]+) parity=(\d+)$',log,re.M)
    require(len(fields)==1,'RESTART COPPER RECORD')
    require(decode_copper((run/'restart-copper.bin').read_bytes(),int(fields[0][0],16),int(fields[0][1]))==(reference/'gameplay-first-frame-2.rgb').read_bytes(),'RESTART COPPER PALETTE/MODE')
    print('GAMEPLAY RESTART PASS: original command, two paired completed frames, restoration')


def controls(run, reference, setup):
    # A passing untouched copy is required before any rejection counts.
    with tempfile.TemporaryDirectory(prefix='restart-controls-',dir=ROOT/'tmp') as folder:
        target=Path(folder)
        names=['gdb-out.log','build-receipt.json','PrinceOfPersia2.elf',
               'logs/debug.uae','dh1/Shell.done','dh1/PrinceOfPersia2','dh1/video-transfer.bin','restart-copper.bin']
        names += [label+'-'+suffix+'.bin' for label in ('first-room','restart')
                  for suffix in ('screen','context','a5','planes','colors')]
        for name in names:
            p=target/name;p.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(run/name,p)
        check(target,reference,setup)
        mutations=[
            ('gdb-out.log',lambda b:b.replace(b'GAMEPLAY / RESTART COMPLETE',b'GAMEPLAY / INCOMPLETE')),
            ('gdb-out.log',lambda b:b.replace(b'rooms=2',b'rooms=1')),
            ('gdb-out.log',lambda b:b.replace(b'menu=01930009',b'menu=00000000')),
            ('gdb-out.log',lambda b:b.replace(b'calls=1/1',b'calls=0/0')),
            ('gdb-out.log',lambda b:b.replace(b'code=1 input=1',b'code=0 input=1')),
            ('gdb-out.log',lambda b:b.replace(b'heap=1',b'heap=0')),
            ('logs/debug.uae',lambda b:b.replace(b'cpu_model=68030',b'cpu_model=68040')),
            ('build-receipt.json',lambda b:b.replace(b'POP2_GAMEPLAY_RESTART_PROBE',b'POP2_OTHER_PROBE')),
            ('dh1/PrinceOfPersia2',lambda b:b[:-1]),
            ('dh1/Shell.done',lambda b:b'SHELL PASS\n'),
            ('restart-screen.bin',lambda b:b[:48*640+65]+bytes([b[48*640+65]^1])+b[48*640+66:]),
            ('restart-planes.bin',lambda b:bytes([b[0]^1])+b[1:]),
            ('restart-colors.bin',lambda b:b[:10]+bytes([b[10]^1])+b[11:]),
            ('restart-context.bin',lambda b:b[:0x218a]+bytes([b[0x218a]^1])+b[0x218b:]),
            ('restart-a5.bin',lambda b:b[:-0x18fc]+bytes([b[-0x18fc]^1])+b[-0x18fb:]),
        ]
        for name,mutate in mutations:
            p=target/name;good=p.read_bytes();bad=mutate(good)
            require(bad!=good,'RESTART CONTROL DID NOT CHANGE EVIDENCE')
            p.write_bytes(bad)
            try:check(target,reference,setup)
            except (RuntimeError,ValueError,AssertionError):pass
            else:raise RuntimeError('RESTART / ACCEPTED ALTERED EVIDENCE '+name)
            finally:p.write_bytes(good)
        print('GAMEPLAY RESTART CONTROLS PASS:',len(mutations))


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('run',type=Path);p.add_argument('reference',type=Path)
    p.add_argument('--setup',required=True)
    p.add_argument('--controls',action='store_true')
    a=p.parse_args();check(a.run,a.reference,a.setup)
    if a.controls:controls(a.run,a.reference,a.setup)
