"""Release save/reload observation through original routines and guest input."""
import hashlib
import json


def observer(action,c2,address):
    if action is None:return ''
    assert action in ('save','load')
    label=action.upper();key='21' if action=='save' else '18'
    picker=7001 if action=='save' else 7002
    returned=c2+(0x143e if action=='save' else 0x11e2)
    script=f'''disable 4
    break *{address('pop2UIOverlayShown'):#x}
    break *{returned:#x}
    monitor key 66 1
    monitor key {key} 1
    continue
    printf "{label} DIALOG id=%lu pc=%lx\\n",*(unsigned long*){address('g_uiOverlay'):#x},$pc
    dump binary memory {action}-dialog.bin {address('g_uiOverlayPixels'):#x} {address('g_uiOverlayPixels')+640*480:#x}
    if $pc != {address('pop2UIOverlayShown'):#x} || *(unsigned long*){address('g_uiOverlay'):#x} != {picker}
     quit 1
    end
    printf "{label} PICKER shown={picker}\\n"
    monitor key {key} 0
    monitor key 66 0
    disable 9
    monitor key 44 1
    continue
    if $pc != {returned:#x} || ($d0&0xffff) != 1
     quit 1
    end
    printf "{label} COMPLETED result=1\\n"
    dump binary memory {action}-return-a5.bin $world-22266 $world
    monitor key 44 0
    disable 10
    '''
    if action=='load':
        script+=f'''break *{c2+0x6058:#x}
    condition 11 ($d0&0xffff)==0xfffe
    continue
    if $pc != {c2+0x6058:#x}
     quit 1
    end
    dump binary memory loaded-first-a5.bin $world-22266 $world
    printf "LOAD FIRST level=%u room=%u\\n",*(unsigned short*)($world-0x4fd2),*(unsigned short*)($world-0x4fc2)
    disable 11
    break *{c2+0x68b2:#x}
    continue
    if $pc != {c2+0x68b2:#x}
     quit 1
    end
    printf "LOAD UPDATE completed=1\\n"
    disable 12
    '''
    return script+'enable 4\n'


def check(base):
    options=base/'gameplay-options.json'
    action=json.loads(options.read_text()).get('save_action') if options.exists() else None
    if action is None:return
    assert action in ('save','load'), 'SAVE / ACTION'
    log=(base/'gdb.log').read_text()
    label=action.upper();picker=7001 if action=='save' else 7002
    for marker in (f'{label} PICKER shown={picker}',f'{label} COMPLETED result=1'):
        assert log.count(marker)==1, 'SAVE / ORIGINAL ROUTINE COMPLETION '+marker
    assert (base/(action+'-return-a5.bin')).stat().st_size==22266, 'SAVE / WORLD EXTENT'
    # The original-paired save fixture establishes this document layout.
    assert (base/'game/Level 1').stat().st_size==100, 'SAVE / DOCUMENT EXTENT'
    assert (base/'game/Level 1.rsrc').read_bytes()==b'', 'SAVE / RESOURCE FORK'
    metadata=(base/'game/Level 1.finfo').read_bytes()
    assert len(metadata)==32 and metadata[:4]==b'AFI1', 'SAVE / METADATA'
    checksum=2166136261
    for byte in metadata[:28]:checksum=((checksum^byte)*16777619)&0xffffffff
    assert int.from_bytes(metadata[28:],'big')==checksum, 'SAVE / METADATA CHECKSUM'
    from extract_original_data import ROOT,pins
    expected=pins(ROOT/'manifest/gameplay-save-reference.sha256')['saved-Level-1.type-creator']
    assert hashlib.sha256(metadata[4:12]).hexdigest()==expected, 'SAVE / ORIGINAL TYPE AND CREATOR'
    if action=='load':
        saved=(base/'save-input-a5.bin').read_bytes()
        moved=(base/'moved-a5.bin').read_bytes()
        assert len(saved)==len(moved)==22266, 'SAVE / INPUT WORLD EXTENT'
        assert saved[-0x50d2:-0x5094]!=moved[-0x50d2:-0x5094], 'SAVE / MOVEMENT CONTROL'
        for marker in ('LOAD FIRST level=1 room=4','LOAD UPDATE completed=1'):
            assert log.count(marker)==1, 'SAVE / RESUMED GAMEPLAY '+marker
        assert (base/'loaded-first-a5.bin').stat().st_size==22266, 'SAVE / LOADED WORLD EXTENT'
        hashes=json.loads((base/'save-input.json').read_text())
        assert set(hashes)=={'Level 1','Level 1.rsrc','Level 1.finfo'}, 'SAVE / INPUT SET'
        for name,digest in hashes.items():
            assert hashlib.sha256((base/'game'/name).read_bytes()).hexdigest()==digest, 'SAVE / PERSISTED BYTES '+name
    print('RELEASE '+label+' PASS: original routine returned success; persisted forks and metadata verified')


def controls(source):
    import shutil,tempfile
    from pathlib import Path
    from extract_original_data import ROOT
    action=json.loads((source/'gameplay-options.json').read_text())['save_action']
    assert action in ('save','load')
    with tempfile.TemporaryDirectory(prefix='release-save-controls-',dir=ROOT/'tmp') as temp:
        base=Path(temp)
        names=['gameplay-options.json','gdb.log',action+'-return-a5.bin',
               'game/Level 1','game/Level 1.rsrc','game/Level 1.finfo']
        if action=='load':names+=['save-input.json','save-input-a5.bin','moved-a5.bin','loaded-first-a5.bin']
        for name in names:
            dest=base/name;dest.parent.mkdir(exist_ok=True,parents=True)
            shutil.copyfile(source/name,dest)
        check(base)
        cases=[('gdb.log',lambda b:b.replace(b'COMPLETED result=1',b'COMPLETED result=0')),
               ('game/Level 1',lambda b:b[:-1]),
               ('game/Level 1.finfo',lambda b:b[:12]+bytes([b[12]^1])+b[13:])]
        if action=='load':
            cases += [('gdb.log',lambda b:b.replace(b'LOAD UPDATE completed=1',b'LOAD UPDATE missing')),
                      ('game/Level 1',lambda b:bytes([b[0]^1])+b[1:])]
        for name,mutate in cases:
            path=base/name;before=path.read_bytes();path.write_bytes(mutate(before))
            try:check(base)
            except AssertionError:pass
            else:raise AssertionError('SAVE / DAMAGED EVIDENCE ACCEPTED '+name)
            path.write_bytes(before)
    print('RELEASE SAVE REJECTION CONTROLS PASS:',len(cases))


if __name__=='__main__':
    import argparse
    from pathlib import Path
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run',type=Path)
    parser.add_argument('--controls',action='store_true')
    args=parser.parse_args()
    from whdload_gameplay import check as gameplay_check
    gameplay_check(args.run)
    if args.controls:controls(args.run)
