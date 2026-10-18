"""Observe the original sound gate preceding the level-thirteen shadow."""
import argparse
from pathlib import Path
from route_flame_observer import generate
import trap_census


def shadow_gate():
    # CODE 10 is loaded by the level-thirteen entry, after the initial room.
    # Reuse the existing flame observer's guarded arming checkpoint.
    script = generate(observe=False)
    assert script.endswith('continue\n')
    setup = ''
    for segment, offsets in ((4, (0x3c68, 0x3c76, 0x3c96)), (10, (0x2b14,))):
        for offset in offsets:
            word = int.from_bytes(trap_census.CODE[segment][offset:offset+2], 'big')
            setup += f'''set $shadow_gate_site=g_coreSegments[{segment}][0]+0x{offset:x}
if *(unsigned short*)$shadow_gate_site!=0x{word:x}
 echo SHADOW GATE / ORIGINAL BYTES\\n
 quit 1
end
break *$shadow_gate_site
commands
 silent
 if *(unsigned short*)(g_coreA5-0x4fd2)==13 && *(unsigned short*)(g_coreA5-0x349a)<=160
  printf "SHADOW_GATE segment={segment} site={offset:04X} step=%04X result=%04X cue1=%04X cue2=%04X\\n",*(unsigned short*)(g_coreA5-0x349a),$d0&65535,*(unsigned short*)(g_coreA5-0x439a),*(unsigned short*)(g_coreA5-0x4398)
 end
 continue
end
'''
    return script[:-len('continue\n')]+setup+'continue\n'


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    args.output.write_text(shadow_gate())
