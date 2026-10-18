"""Generate an opt-in, read-only native exit-gate observer from the route observer."""
import argparse
from pathlib import Path
from extract_original_data import DESTINATION, verify_output
import trap_census

ROOT = Path(__file__).resolve().parents[1]


def generate(audio=False):
    verify_output(DESTINATION)
    trap_census.load(DESTINATION/'Prince of Persia 2.rsrc')
    script = (ROOT/'amiga/gameplay_route.gdb').read_text()
    # Reuse its initial checkpoint capture verbatim. Stop once before installing
    # original-code breakpoints, after CODE 2 has been loaded and byte-verified.
    marker = 'break pop2GameplayRouteCheckpoint\ncommands\n'
    assert script.count(marker) == 1 and script.endswith('continue\n')
    assert [line for line in script.split(marker)[0].splitlines()
            if line.startswith('break ')] == ['break pop2UIOverlayShown']
    capture = script.split(marker)[1].split('\n continue\nend\n', 1)[0]
    capture = capture.removeprefix(' silent\n')
    setup = '''disable 2
tbreak pop2GameplayRouteCheckpoint
continue
if $pc != pop2GameplayRouteCheckpoint
 echo EXIT OBSERVER / INITIAL CHECKPOINT NOT REACHED\\n
 quit 1
end
enable 2
'''+capture+'\nset $exitbase=g_coreSegments[2][0]\n'
    for offset in (0x6134, 0x6144, 0x6152, 0x61ce):
        word = int.from_bytes(trap_census.CODE[2][offset:offset+2], 'big')
        setup += f'''if *(unsigned short*)($exitbase+0x{offset:x}) != 0x{word:x}
 echo EXIT OBSERVER / ORIGINAL BYTES\\n
 quit 1
end
break *($exitbase+0x{offset:x})
commands
 silent
 if *(unsigned short*)(g_coreA5-0x4f92) != *(unsigned short*)(g_coreA5-0x4fd2)
  printf "ROUTE_EXIT site={offset:04X} step=%u level=%u next=%u cue=%04X result=%04X\\n",*(unsigned short*)(g_coreA5-0x349a),*(unsigned short*)(g_coreA5-0x4fd2),*(unsigned short*)(g_coreA5-0x4f92),*(unsigned short*)(g_coreA5-0x439a),$d0&65535
 end
 continue
end
'''
    if audio:
        setup += '''set $route_audio=*(unsigned long*)(g_coreA5-0xf24)
if *(unsigned long*)$route_audio != 0xaffc4e75
 echo EXIT OBSERVER / AUDIO VENEER BYTES\\n
 quit 1
end
set $route_audio_clock=0
break *($route_audio+2)
commands
 silent
 set $route_audio_selector=*(unsigned long*)($sp+4)
 if $route_audio_selector==15
  set $route_audio_clock=$d0
 end
 printf "ROUTE_AUDIO step=%u selector=%u result=%08X last_clock=%08X caller=%08X\\n",*(unsigned short*)(g_coreA5-0x349a),$route_audio_selector,$d0,$route_audio_clock,*(unsigned long*)$sp
 if $route_audio_selector==9 || $route_audio_selector==12 || $route_audio_selector==17
  set $route_audio_packet=*(unsigned long*)($sp+8)
  printf "ROUTE_AUDIO_PACKET words=%04X/%04X/%04X/%04X/%04X\\n",*(unsigned short*)$route_audio_packet,*(unsigned short*)($route_audio_packet+2),*(unsigned short*)($route_audio_packet+4),*(unsigned short*)($route_audio_packet+6),*(unsigned short*)($route_audio_packet+8)
 end
 continue
end
'''
    return script[:-len('continue\n')]+setup+'continue\n'


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    parser.add_argument('--audio', action='store_true', help='Observe the owned audio veneer return without guest instrumentation')
    args = parser.parse_args()
    args.output.write_text(generate(args.audio))
