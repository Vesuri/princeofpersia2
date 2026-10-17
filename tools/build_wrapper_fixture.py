"""Link the real diagnostic wrappers with destructive, authored ABI callbacks."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess

from aprof_report import elf_section
from extract_original_data import ROOT

WRAPPERS = ['pop2_integration_title', 'pop2_integration_opening',
            'pop2_integration_room', 'pop2_gameplay_route_hall',
            'pop2_gameplay_route_ending', 'pop2_gameplay_route_frame',
            'pop2_gameplay_saved', 'pop2_gameplay_loaded',
            'pop2_integration_death', 'pop2_integration_death_return']
TARGETS = ['g_integrationTitleTarget', 'g_integrationOpeningReturn',
           'g_integrationRoomReturn', 'g_routeHallReturn', 'g_routeEndingReturn',
           'g_routeReturn', 'g_gameplaySaveReturn', 'g_gameplayLoadReturn',
           'g_integrationDeathTarget', 'g_integrationDeathReturn']


def build(out, source):
    from scaffold_inputs import verify
    verify("build")
    out = out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    source = source.resolve()
    stub = ROOT/'tools/wrapper_ccr_fixture.s'
    flags = ['-mcpu=68020', '--register-prefix-optional',
             '--defsym', 'POP2_INTEGRATION_PROBE=1',
             '--defsym', 'POP2_GAMEPLAY_ROUTE_PROBE=1',
             '--defsym', 'POP2_GAMEPLAY_SAVE_PROBE=1']
    subprocess.run(['m68k-amiga-elf-as', *flags, str(source), '-o', str(out/'wrappers.o')], cwd=ROOT/'amiga', check=True)
    subprocess.run(['m68k-amiga-elf-as', *flags, str(stub), '-o', str(out/'fixture.o')], check=True)
    script = out/'fixture.ld'
    script.write_text('SECTIONS { . = 0; .text : { *(.text*) } . = ALIGN(4); '
                      '.data : { *(.data*) } /DISCARD/ : { *(.debug*) *(.comment) } }\n')
    elf = out/'fixture.elf'
    subprocess.run(['m68k-amiga-elf-ld', '--gc-sections', '--emit-relocs',
                    '-e', WRAPPERS[0], '-T', str(script), str(out/'wrappers.o'),
                    str(out/'fixture.o'), '-o', str(elf)], check=True)
    text = elf_section(elf, '.text')
    (out/'image.bin').write_bytes(text + b'\0' * (-len(text) % 4) + elf_section(elf, '.data'))
    symbols = {}
    for line in subprocess.check_output(['m68k-amiga-elf-objdump', '-t', str(elf)], text=True).splitlines():
        fields = line.split()
        if len(fields) >= 5 and fields[0].isalnum() and len(fields[0]) == 8:
            symbols[fields[-1]] = int(fields[0], 16)
    relocations = []
    for section in ('.rela.text', '.rela.data'):
        for offset, info, addend in struct.iter_unpack('>IIi', elf_section(elf, section)):
            kind = info & 255
            assert kind in (1, 4, 5, 6), ('unexpected fixture relocation', kind)
            if kind == 1: relocations.append(offset)
    image = (out/'image.bin').read_bytes()
    assert len(image) < 4096 and len(relocations) == len(set(relocations))
    names = WRAPPERS + TARGETS + ['wrapperFixtureDone', 'wrapperFixtureArgument', 'wrapperFixtureHook']
    symbols = {name: symbols[name] for name in names}
    assert all(0 <= value < len(image) for value in symbols.values())
    assert all(0 <= offset <= len(image)-4 for offset in relocations)
    metadata = dict(size=len(image), relocations=sorted(relocations), symbols=symbols,
                    wrappers=WRAPPERS, targets=TARGETS,
                    source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
                    stub_sha256=hashlib.sha256(stub.read_bytes()).hexdigest(),
                    image_sha256=hashlib.sha256(image).hexdigest())
    (out/'metadata.json').write_text(json.dumps(metadata, indent=2)+'\n')
    (out/'metadata.lua').write_text('return {size='+str(len(image))+',relocations={'+
        ','.join(map(str, sorted(relocations)))+'},symbols={'+
        ','.join('['+json.dumps(k)+']='+str(v) for k,v in symbols.items())+'},wrappers={'+
        ','.join(json.dumps(n) for n in WRAPPERS)+'}}\n')
    header = 'static const unsigned char fixture_image[] = {'+','.join(map(str,image))+'};\n'
    for name, values in [('relocations', sorted(relocations)),
                         ('wrappers', [symbols[n] for n in WRAPPERS]),
                         ('targets', [symbols[n] for n in TARGETS])]:
        header += 'static const unsigned long fixture_'+name+'[] = {'+','.join(map(str,values))+'};\n'
    header += '#define FIXTURE_ARGUMENT '+str(symbols['wrapperFixtureArgument'])+'\n'
    header += '#define FIXTURE_HOOK '+str(symbols['wrapperFixtureHook'])+'\n'
    (out/'fixture.h').write_text(header)
    print('WRAPPER FIXTURE BUILT:', len(image), 'bytes;', len(relocations), 'relocations')


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('output', type=Path)
    p.add_argument('--source', type=Path, default=ROOT/'src/platform/amiga/MacEntry.s')
    a = p.parse_args()
    build(a.output, a.source)
