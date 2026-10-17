"""Release gameplay and normal-quit observer, without compiled-in game probes."""
from pathlib import Path
import struct,json,hashlib
from extract_original_data import ROOT

def prepare(base):
    elf=(base/'PrinceOfPersia2.elf').read_bytes()
    assert hashlib.sha256(elf).hexdigest()==json.loads((base/'receipt.json').read_text())['elf']
    u32=lambda b,o:struct.unpack_from('>I',b,o)[0]
    o=u32(elf,32);width,n=struct.unpack_from('>HH',elf,46);sections=[struct.unpack_from('>10I',elf,o+i*width) for i in range(n)]
    syms={}
    for s in sections:
     if s[1]!=2:continue
     strings=sections[s[6]];st=elf[strings[4]:strings[4]+strings[5]]
     for i in range(s[4],s[4]+s[5],s[9]):
      name,val,size,info,other,seg=struct.unpack_from('>IIIBBH',elf,i)
      syms[st[name:].split(b'\0')[0].decode()]=(val,seg)
    fast=(base/'before-fast.bin').read_bytes();chip=(base/'before-chip.bin').read_bytes()
    def read(a,n):
     if 0<=a and a+n<=len(chip):return chip[a:a+n]
     assert 0x200000<=a and a+n<=0xa00000,hex(a)
     return fast[a-0x200000:a-0x200000+n]
    def long(a):return int.from_bytes(read(a,4),'big')
    magic=b'POP2WHDR'+bytes.fromhex('00020000');found=[];off=0
    while True:
     i=fast.find(magic,off)
     if i<0:break
     if u32(fast,i+12):found.append(i+0x200000)
     off=i+4
    assert len(found)==1,found
    val,seg=syms['pop2WHDLoadBinding'];bases={seg:found[0]-(val-sections[seg][3])}
    allocated=[i for i,s in enumerate(sections) if s[2]&2 and s[5]]
    for nextseg in allocated[allocated.index(seg)+1:]:
     bases[nextseg]=long(bases[seg]-4)*4+4;seg=nextseg
    starts=[];at=fast.find(bytes.fromhex('598f2f0a45f9'))
    while at>=0:
     if u32(fast,at+0x3a)==0x200000+at+syms['main'][0]:starts.append(at+0x200000)
     at=fast.find(bytes.fromhex('598f2f0a45f9'),at+1)
    assert len(starts)==1,starts
    bases[syms['main'][1]]=starts[0]
    def address(name):
     v,s=syms[name];return bases[s]+v-sections[s][3]
    segments=address('g_coreSegments');c2=long(segments+16);c15=long(segments+120)
    assert c2 and c15,(hex(c2),hex(c15))
    a5=long(address('g_coreA5'));assert a5
    audio=long(a5-0xf24)
    assert read(audio,4)==bytes.fromhex('affc4e75'), 'QUIT / AUDIO ENTRY'
    options=json.loads((base/'gameplay-options.json').read_text())
    action=options.get('save_action')
    updates=16 if action=='load' else 11 if action=='save' else 8
    from whdload_save import observer
    file_route=observer(options.get('save_action'),c2,address,mmu=options.get('mmu_functional',False))
    s=f'''set $world={a5:#x}
    set $c2={c2:#x}
    set $c15={c15:#x}
    monitor key 45 1
    monitor fields 12
    monitor key 45 0
    break *{c15+0x6fec:#x}
    condition 1 *(unsigned short*)($sp+4)>=3 && *(unsigned short*)($world-0x1f6e)==0x72
    continue
    if $pc != {c15+0x6fec:#x}
     quit 1
    end
    printf "PLAY TITLE level=%u room=%u\\n",*(unsigned short*)($world-0x4fd2),*(unsigned short*)($world-0x4fc2)
    disable 1
    monitor key 45 0
    monitor key 40 1
    monitor fields 100
    monitor key 40 0
    break *{c2+0x68b2:#x}
    continue
    if $pc != {c2+0x68b2:#x}
     quit 1
    end
    printf "PLAY FIRST level=%u room=%u step=%u\\n",*(unsigned short*)($world-0x4fd2),*(unsigned short*)($world-0x4fc2),*(unsigned short*)($world-0x349a)
    monitor key 40 0
    disable 2
    set $context=*(unsigned long*)($world-0x5014)
    dump binary memory first-context.bin $context $context+0x2200
    dump binary memory first-a5.bin $world-22266 $world
    monitor key 4e {0 if action=='save' else 1}
    break *{c2+0x68b2:#x}
    ignore 3 {updates-1}
    continue
    if $pc != {c2+0x68b2:#x}
     quit 1
    end
    printf "PLAY MOVED level=%u room=%u step=%u\\n",*(unsigned short*)($world-0x4fd2),*(unsigned short*)($world-0x4fc2),*(unsigned short*)($world-0x349a)
    dump binary memory moved-context.bin $context $context+0x2200
    dump binary memory moved-a5.bin $world-22266 $world
    monitor key 4e 0
    disable 3
    break *{address('pop2UIOverlayShown'):#x}
    break *{address('pop2ScaffoldRestored'):#x}
    break *{c2+0x3e36:#x}
    commands 6
     silent
     set $dispose_sp=$sp
     set $window=*(unsigned long*)$sp
     dump binary memory quit-window-before.bin $window $window+156
     disable 6
     continue
    end
    break *{c2+0x3e38:#x}
    commands 7
     silent
     if $sp != $dispose_sp+4
      quit 1
     end
     set $port_slot=*(unsigned long*){address('_ZN13NativeDisplay12_GLOBAL__N_1L7thePortE'):#x}
     set $port=*(unsigned long*)$port_slot
     if $port == $window
      quit 1
     end
     dump binary memory quit-port-after.bin $port $port+108
     printf "QUIT WINDOW RETURN stack=4 fallback=1\\n"
     disable 7
     continue
    end
    break *{audio+2:#x}
    condition 8 *(unsigned long*)($sp+4)==8
    commands 8
     silent
     printf "QUIT AUDIO RETURN selector=8 result=%lu\\n",$d0
     disable 8
     continue
    end
    {'disable 8' if options.get('mmu_functional') else ''}
    {file_route}
    monitor key 66 1
    monitor key 10 1
    continue
    if $pc == {address('pop2UIOverlayShown'):#x}
     printf "PLAY CONFIRM id=%lu\\n",*(unsigned long*){address('g_uiOverlay'):#x}
     if *(unsigned long*){address('g_uiOverlay'):#x} != 405 && *(unsigned long*){address('g_uiOverlay'):#x} != 406
      quit 1
     end
     monitor key 10 0
     monitor key 66 0
     disable 4
     monitor key 44 1
     continue
    end
    if $pc != {address('pop2ScaffoldRestored'):#x}
     quit 1
    end
    printf "QUIT PATCHES load=%lu exit=%lu\\n",*(unsigned long*){address('_ZN12_GLOBAL__N_1L11patchedLoadE'):#x},*(unsigned long*){address('_ZN12_GLOBAL__N_1L11patchedExitE'):#x}
    printf "CORE REASON: "
    x/s *(unsigned long*){address('g_coreReason'):#x}
    printf "CORE STOP trap=%lx seg=%lu offset=%lx\\n",*(unsigned long*){address('g_coreStopTrap'):#x},*(unsigned long*){address('g_coreStopSegment'):#x},*(unsigned long*){address('g_coreStopOffset'):#x}
    monitor key 44 0
    printf "PLAY RESTORED error=%lu view=%u vbi=%u input=%lu\\n",*(unsigned long*){address('g_scaffoldError'):#x},*(unsigned short*){address('g_viewRestored'):#x},*(unsigned short*){address('g_vectorRestored'):#x},*(unsigned long*){address('g_inputVectorRestored'):#x}
    monitor key 10 0
    monitor key 66 0
    '''
    if options.get('quit_key'):
        s=s.split(f'    break *{address("pop2UIOverlayShown"):#x}')[0]+('    monitor key 59 1\n    monitor fields 40\n'
            '    monitor key 59 0\n    monitor fields 80\n    printf \"QUIT F10 INJECTED\\n\"\n')
    (base/'game-next.gdb').write_text(s)
    


def check(base):
    import re
    log=(base/'gdb.log').read_text()
    options=json.loads((base/'gameplay-options.json').read_text()) if (base/'gameplay-options.json').exists() else {}
    quit_key=options.get('quit_key')
    action=options.get('save_action')
    mmu=options.get('mmu_functional',False)
    updates=16 if action=='load' else 11 if action=='save' else 8
    first=re.findall(r'^PLAY FIRST level=1 room=4 step=(\d+)$',log,re.M)
    moved=re.findall(r'^PLAY MOVED level=1 room=4 step=(\d+)$',log,re.M)
    assert len(first)==len(moved)==1 and all(int(v)<65536 for v in first+moved) and (int(moved[0])-int(first[0]))%65536==updates, 'QUIT / GAMEPLAY UPDATE SEQUENCE'
    markers=[]
    if quit_key:
        launch=(base/'boot/s/startup-sequence').read_text()
        assert 'TIMEOUT=' not in launch and 'COREDUMP' not in launch, 'QUIT / ORDINARY F10 OPTIONS'
        assert 'Expert' not in (base/'boot/s/WHDLoad.prefs').read_text(), 'QUIT / ORDINARY F10 PREFERENCES'
        markers+=['QUIT F10 INJECTED']
    else:
        markers+=['PLAY RESTORED error=0 view=1 vbi=1 input=1',
                  'QUIT WINDOW RETURN stack=4 fallback=1',
                  'QUIT PATCHES load=0 exit=0','"CORE / EXIT TO SHELL"']
        if not mmu:markers+=['QUIT AUDIO RETURN selector=8 result=0']
        assert re.search(r'CORE STOP trap=a9f4\b',log), 'QUIT / ORIGINAL EXIT TRAP'
    for marker in markers:
        assert log.count(marker)==1, 'QUIT / MISSING COMPLETION '+marker
    assert (base/'boot/passed').exists() and (base/'boot/returned').exists(), 'QUIT / SHELL RETURN'
    assert not (base/'boot/failed').exists(), 'QUIT / SHELL FAILURE'
    report=base/'game/.whdl_register'
    if report.exists():
        assert report.read_text(encoding='latin1').count('\nReturn OK.\n')==1, 'QUIT / WHDLOAD ERROR'
    if mmu or options.get('cache_memory'):
        assert options.get('setup')=='a1200-040' and options.get('mmu') and not quit_key, 'MMU / OBSERVER SCOPE'
        if mmu:assert action in ('save','load'), 'MMU / FUNCTIONAL SCOPE'
        config=(base/'logs/debug.uae').read_text().splitlines()
        assert all(setting in config for setting in ('cpu_model=68040','mmu_model=68040','cachesize=0','cpu_compatible=true','cpu_data_cache=true')), 'MMU / EFFECTIVE MACHINE'
        registers=report.read_text(encoding='latin1')
        assert 'TC=8000 ' in registers and 'CACR=80008000 ' in registers, 'MMU / ACTIVE TRANSLATION AND CACHES'
        launch=(base/'boot/s/startup-sequence').read_text()
        assert re.search(r'\bMMU\b',launch) and not re.search(r'\bNOMMU\b',launch), 'MMU / WHDLOAD OPTION'
    receipt=json.loads((base/'receipt.json').read_text())
    assert set(receipt['flags'].split())=={'-DPOP2_'+v for v in ('CORE','DISPLAY','INPUT','UI','AUDIO')}, 'QUIT / RELEASE BUILD'
    assert hashlib.sha256((base/'game/PrinceOfPersia2').read_bytes()).hexdigest()==receipt['exe'], 'QUIT / EXECUTABLE IDENTITY'
    for name in ('first','moved'):
        assert (base/(name+'-context.bin')).stat().st_size==0x2200, 'QUIT / CONTEXT EXTENT'
        assert (base/(name+'-a5.bin')).stat().st_size==22266, 'QUIT / WORLD EXTENT'
    result={'result':'WHDLOAD GAMEPLAY AND QUIT PASS','exe':receipt['exe'],'updates':updates,'quit':'F10' if quit_key else 'Amiga-Q'}
    from whdload_save import check as save_check
    save_check(base)
    if mmu:result.update(result='WHDLOAD MMU FILE AND QUIT FUNCTIONAL PASS',audio_return='not observed')
    (base/('mmu-functional.json' if mmu else 'gameplay-quit.json')).write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result))


def run(base, seconds):
    import os,shlex,shutil,subprocess,time
    from whdload_store import emulator
    cache_memory=json.loads((base/'gameplay-options.json').read_text()).get('cache_memory',False)
    emulator(cache_memory)
    shutil.copyfile(ROOT/'amiga/out/PrinceOfPersia2.elf',base/'PrinceOfPersia2.elf')
    for _ in range(100):
        if (base/'debug-port').exists():
            port=(base/'debug-port').read_text().strip()
            if subprocess.run(['lsof','-nP','-iTCP:'+port,'-sTCP:LISTEN'],capture_output=True).returncode==0:break
        time.sleep(.2)
    else:raise AssertionError('QUIT / DEBUGGER LISTENER')
    commands=('set pagination off\nset confirm off\nset remotetimeout 300\n'
              f'target remote 127.0.0.1:{port}\n'+('monitor memory cache\nset stack-cache off\nset code-cache off\n' if cache_memory else '')+
              'monitor fields 1200\n'
              'dump binary memory before-fast.bin 0x200000 0xa00000\n'
              'dump binary memory before-chip.bin 0 0x200000\n'
              'shell python3 '+shlex.quote(str(Path(__file__).resolve()))+' --prepare\n'
              'source game-next.gdb\ndetach\nquit\n')
    (base/'game.gdb').write_text(commands);home=base/'home';home.mkdir(exist_ok=True)
    with (base/'gdb.log').open('w') as out:
        subprocess.run(['m68k-amiga-elf-gdb','--batch','-q','-x','game.gdb'],cwd=base,
                       env=dict(os.environ,HOME=str(home),XDG_CACHE_HOME=str(home)),
                       stdout=out,stderr=out,check=True,timeout=seconds)
    for _ in range(120):
        if (base/'boot/returned').exists():break
        time.sleep(.5)
    check(base)


if __name__=='__main__':
    import sys
    assert sys.argv[1:]==['--prepare']
    prepare(Path.cwd())
