#!/usr/bin/env python3
"""Reject ambiguous generated hexadecimal operands in maintained MAME scripts."""
import argparse
import json
from pathlib import Path
import re
import unittest
ROOT=Path(__file__).resolve().parents[1]
# These exact formats are host patterns, map keys and capture filenames, never debugger operands.
NON_OPERANDS={'INPUT_COUNT=(%x+)','%x:%d','poly-%03u','region-%03u'}
STRINGS=re.compile(r"--[^\n]*|(?P<quote>['\"])(?P<body>(?:\\.|(?!(?P=quote)).)*)(?P=quote)",re.S)
FORMAT=re.compile(r'(?<!%)%(?:0?\d*)[xXdiu]')
def inspect(source):
    errors=[];in_diagnostic=False
    for token in STRINGS.finditer(source):
        body=token.group('body')
        if body is None or body in NON_OPERANDS:continue
        line=source.count('\n',0,token.start())+1
        # A quoted MAME printf format may span several concatenated Lua
        # strings. Preserve that state while excluding diagnostic text.
        operands='';remaining=body
        while remaining:
            if in_diagnostic:
                end=remaining.find('"')
                if end<0:break
                remaining=remaining[end+1:];in_diagnostic=False
            else:
                start=re.search(r'(?:logerror|printf)\s+"',remaining)
                if not start:operands+=remaining;break
                operands+=remaining[:start.start()];remaining=remaining[start.end():];in_diagnostic=True
        line_prefix=source[source.rfind('\n',0,token.start())+1:token.start()]
        if 'print(string.format(' in line_prefix:continue # host diagnostic, not debugger input
        for m in FORMAT.finditer(operands):
            if operands[max(0,m.start()-2):m.start()].lower()=='0x':
                if m.group()[-1] not in 'xX':errors.append((line,'decimal format after hex prefix'))
                continue
            # Lowercase hex is reserved for emitted numeric literals. Also
            # catch uppercase/decimal variants in memory/address expressions.
            lower=m.group().endswith('x')
            address=bool(re.search(r'(?:[@+\-]|(?:temp\d+|[ad][0-7]|pc|sp)\s*[=!]=?)\s*$',operands[:m.start()]))
            if lower or address:errors.append((line,'unprefixed numeric format '+m.group()))
        if re.search(r'b@\([^;]*\)=%s;',operands):errors.append((line,'unprefixed byte-string literal'))
    return errors

def scripts():return sorted(set((ROOT/'tools').glob('mac_*.lua'))|set((ROOT/'tools').glob('mame_*.lua')))
class Checks(unittest.TestCase):
    def test_ambiguous_formats(self):
        for text in ["'d@(base+%x)'", "'d@(base+%X)'", "'d@(base+%d)'", "'d@(base+0x%d)'", "'temp0=%x'", "'d0=%X'", "'b@(%s+0x%x)=%s;'"]:
            with self.subTest(text=text):self.assertTrue(inspect(text))
    def test_literals_and_diagnostics(self):
        for text in ["'d@(base+0x%x)'", "'d@(base+0x%X)'", "'temp0=0x%x'", "'b@(%s+0x%x)=0x%s;'", "'logerror \"address=%08X sp=%X\\n\",sp'", "'INPUT_COUNT=(%x+)'", "'%x:%d'", "-- 'd@(base+%x)'\n", "dump(string.format('poly-%03u',n),a,b)", "dump(string.format('region-%03u',n),a,b)"]:
            with self.subTest(text=text):self.assertFalse(inspect(text))
    def test_dump_mutation(self):
        source="dbg:command(string.format('save tmp/frame.bin,base+0x%x,0x100',offset))"
        self.assertFalse(inspect(source))
        self.assertTrue(inspect(source.replace('+0x%x','+%x',1)))
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--syntax-wrapper',type=Path);p.add_argument('--then-script',default='tools/mac_reference.lua');a=p.parse_args()
    if not unittest.TextTestRunner().run(unittest.defaultTestLoader.loadTestsFromTestCase(Checks)).wasSuccessful():raise SystemExit(1)
    errors=[f'{path.relative_to(ROOT)}:{line}: {reason}' for path in scripts() for line,reason in inspect(path.read_text())]
    if errors:raise SystemExit('FAIL MAME literals:\n'+'\n'.join(errors))
    if a.syntax_wrapper:
        paths=[str(path.relative_to(ROOT)) for path in scripts()]
        a.syntax_wrapper.write_text('local paths={'+','.join(json.dumps(path) for path in paths)+'}\nfor _,path in ipairs(paths) do assert(loadfile(path)) end\nprint("PASS MAME script syntax files='+str(len(paths))+'")\ndofile('+json.dumps(a.then_script)+')\n')
    print(f'PASS MAME literal audit: {len(scripts())} scripts; explicit generated numbers and byte strings')
