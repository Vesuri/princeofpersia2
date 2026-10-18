"""Generate an opt-in read-only observer for the original sacred-flame update."""
from pathlib import Path
from extract_original_data import ROOT, DESTINATION, verify_output
import trap_census


def generate(events=False):
    verify_output(DESTINATION)
    trap_census.load(DESTINATION/'Prince of Persia 2.rsrc')
    script = (ROOT/'amiga/gameplay_route.gdb').read_text()
    marker = 'break pop2GameplayRouteCheckpoint\ncommands\n'
    assert script.count(marker) == 1 and script.endswith('continue\n')
    before, tail = script.split(marker)
    capture, after = tail.split('\n continue\nend\n', 1)
    # Let the ordinary observer capture every checkpoint. Pause its command
    # list only at the first level-thirteen checkpoint, when CODE 10 is loaded.
    script = before+'set $flame_armed=0\n'+marker+capture+'''
 if $flame_armed || *(unsigned short*)(g_coreA5-0x4fd2)!=13
  continue
 end
end
'''+after
    setup = '''if *(unsigned short*)(g_coreA5-0x4fd2)!=13
 echo FLAME OBSERVER / INITIAL CHECKPOINT NOT REACHED\\n
 quit 1
end
set $flamebase=g_coreSegments[10][0]
'''
    for offset in (() if events else (0x2cc2, 0x2d48)):
        word = int.from_bytes(trap_census.CODE[10][offset:offset+2], 'big')
        setup += f'''if *(unsigned short*)($flamebase+0x{offset:x})!=0x{word:x}
 echo FLAME OBSERVER / ORIGINAL BYTES\\n
 quit 1
end
break *($flamebase+0x{offset:x})
commands
 silent
 if *(unsigned short*)(g_coreA5-0x4fd2)==13 && *(unsigned short*)(g_coreA5-0x349a)<=640
  printf "ROUTE_FLAME site={offset:04X} step=%u state=%04X next=%04X shape=%08X tile=%04X\\n",*(unsigned short*)(g_coreA5-0x349a),*(unsigned short*)(g_coreA5-0x4818),$d4&65535,*(unsigned long*)(g_coreA5-0x4c9e),*(unsigned short*)(*(unsigned long*)(g_coreA5-0x5014)+0x914)
 end
 continue
end
'''
    if events:
        for offset in (0x2cc2, 0x29f8, 0x2ab4):
            word = int.from_bytes(trap_census.CODE[10][offset:offset+2], 'big')
            window = ' && *(unsigned short*)(g_coreA5-0x349a)>=160 && *(unsigned short*)(g_coreA5-0x349a)<=240' if offset == 0x2cc2 else ' && *(unsigned short*)(g_coreA5-0x349a)<=640'
            setup += f'''if *(unsigned short*)($flamebase+0x{offset:x})!=0x{word:x}
 echo FLAME EVENTS / ORIGINAL BYTES\\n
 quit 1
end
break *($flamebase+0x{offset:x})
commands
 silent
 if *(unsigned short*)(g_coreA5-0x4fd2)==13{window}
  set $flame_step=*(unsigned short*)(g_coreA5-0x349a)
  set $flame_context=*(unsigned long*)(g_coreA5-0x5014)
  eval "dump binary memory @RUN@/flame-event-{offset:04X}-%u-a5.bin g_coreA5-22266 g_coreA5",$flame_step
  eval "dump binary memory @RUN@/flame-event-{offset:04X}-%u-context.bin $flame_context $flame_context+0x2200",$flame_step
  printf "FLAME_EVENT site={offset:04X} step=%u frame=%04X player=%04X\\n",$flame_step,*(unsigned short*)(g_coreA5-0x504c),*(unsigned short*)(g_coreA5-0x50c8)
 end
 continue
end
'''
    return script+setup+'set $flame_armed=1\ncontinue\n'


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    parser.add_argument('--events', action='store_true')
    args = parser.parse_args()
    args.output.write_text(generate(args.events))
