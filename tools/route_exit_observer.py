"""Generate an opt-in, read-only native exit-gate observer from the route observer."""
import argparse
from pathlib import Path
from extract_original_data import DESTINATION, verify_output
import trap_census

ROOT = Path(__file__).resolve().parents[1]


def generate(audio=False, random=False, opponents=False, music_time=False, exit_rng=False, rng_start=1490, rng_end=1545):
    assert not music_time or audio, 'MUSIC TIME / AUDIO OBSERVER REQUIRED'
    assert not (random and exit_rng), 'RANDOM OBSERVER / SELECT ONE WINDOW'
    assert 0 <= rng_start <= rng_end <= 65535, 'EXIT RNG / WINDOW'
    verify_output(DESTINATION)
    trap_census.load(DESTINATION/'Prince of Persia 2.rsrc')
    script = (ROOT/'amiga/gameplay_route.gdb').read_text()
    if opponents:
        marker = ' eval "dump binary memory @RUN@/route-%u-context.bin $context $context+0x2200",g_routeCheckpoint\n'
        assert script.count(marker) == 1
        script = script.replace(marker, marker+''' set $room_index=0
 while $room_index<32
  if *(unsigned short*)($context+0x21a6+$room_index*0xc0)>5
   echo OPPONENT STATE / ROOM COUNT\\n
   quit 1
  end
  set $room_index=$room_index+1
 end
 eval "dump binary memory @RUN@/route-%u-opponents.bin $context+0x21a6 $context+0x39a6",g_routeCheckpoint
 printf "ROUTE_RANDOM_MAP segment=8 base=%08X\\n",g_coreSegments[8][0]
''')
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
    if random or exit_rng:
        setup += 'set $route_random_base=g_coreSegments[17][0]\n'
        for offset in (0x1c4e, 0x6e1e):
            word = int.from_bytes(trap_census.CODE[17][offset:offset+2], 'big')
            setup += f'''if *(unsigned short*)($route_random_base+0x{offset:x})!=0x{word:x}
 echo RANDOM OBSERVER / ORIGINAL BYTES\\n
 quit 1
end
break *($route_random_base+0x{offset:x})
commands
 silent
 if {f'*(unsigned short*)(g_coreA5-0x349a)>={rng_start} && *(unsigned short*)(g_coreA5-0x349a)<={rng_end}' if exit_rng else '*(unsigned short*)(g_coreA5-0x4fd2)==4'}
  printf "{'EXIT_RNG step=%04X level=%04X' if exit_rng else 'ROUTE_RANDOM step=%u'} seed=%08X result=%04X site={offset:04X} caller=%08X\\n",*(unsigned short*)(g_coreA5-0x349a),{'*(unsigned short*)(g_coreA5-0x4fd2),' if exit_rng else ''}*(unsigned long*)(g_coreA5-0x18fc),*(unsigned short*)$sp,*(unsigned long*)($a6+4)
 end
 continue
end
'''
        for segment in (4, 5, 8, 17):
            setup += f'printf "ROUTE_RANDOM_MAP segment={segment} base=%08X\\n",g_coreSegments[{segment}][0]\n'
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
    if music_time:
        marker = ' if $route_audio_selector==9 || $route_audio_selector==12 || $route_audio_selector==17\n'
        assert setup.count(marker) == 1
        setup = setup.replace(marker, ''' if $route_audio_selector==9 || $route_audio_selector==12
  set $route_note_packet=*(unsigned long*)($sp+8)
  printf "ROUTE_NOTE_TIME selector=%u due=%08X origin=%08X\\n",$route_audio_selector,*(unsigned long*)($route_note_packet+10),*(unsigned long*)(g_coreA5-0x11d4)
 end
'''+marker)
    if exit_rng:
        marker = ' printf "CORE STOP trap='
        assert script.count(marker) == 1
        maps = ''.join(f' printf "EXIT_RNG_MAP segment={segment} base=%08X size=%u\\n",g_coreSegments[{segment}][0],g_coreSegments[{segment}][1]\n' for segment in range(1, 24))
        script = script.replace(marker, maps+marker)
    return script[:-len('continue\n')]+setup+'continue\n'


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    parser.add_argument('--audio', action='store_true', help='Observe the owned audio veneer return without guest instrumentation')
    parser.add_argument('--random', action='store_true', help='Observe original RNG returns during level four without guest instrumentation')
    parser.add_argument('--opponents', action='store_true', help='Capture the persistent per-room opponent records at checkpoints')
    parser.add_argument('--music-time', action='store_true', help='With --audio, observe queued note timestamps and their TickCount origin')
    parser.add_argument('--exit-rng', action='store_true', help='Observe RNG calls in the selected exit window, including next-level entry')
    parser.add_argument('--rng-start', type=int, default=1490)
    parser.add_argument('--rng-end', type=int, default=1545)
    args = parser.parse_args()
    args.output.write_text(generate(args.audio, args.random, args.opponents, args.music_time, args.exit_rng, args.rng_start, args.rng_end))
