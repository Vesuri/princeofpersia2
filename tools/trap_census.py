#!/usr/bin/env python3
"""Flow-following census adapted from Alone in the Dark, without CREL.

Offsets include resource headers. Unknown transfers remain explicit.
"""
import argparse
import collections
import re
import struct
from pathlib import Path
import _amigaxdev
from amigaxdev.mac.resource_fork import read_resource_fork
# Verify the disassembler implementation before importing/using it.
import hashlib
import importlib.util
_capstone_root=Path(importlib.util.find_spec('capstone').origin).parent
for _line in (Path(__file__).resolve().parents[1]/'manifest/capstone.sha256').read_text().splitlines():
    _digest,_name=_line.split('  ',1)
    if hashlib.sha256((_capstone_root/_name).read_bytes()).hexdigest()!=_digest:
        raise ValueError('CAPSTONE HASH / '+_name)
from capstone import Cs, CS_ARCH_M68K, CS_MODE_M68K_040, CsError
from capstone.m68k import (M68K_OP_BR_DISP, M68K_OP_MEM, M68K_OP_IMM, M68K_OP_REG,
                           M68K_AM_PCI_DISP, M68K_AM_PCI_INDEX_8_BIT_DISP,
                           M68K_AM_ABSOLUTE_DATA_SHORT, M68K_AM_ABSOLUTE_DATA_LONG,
                           M68K_AM_REGI_ADDR_DISP)
import _amigaxdev  # noqa: F401
from amigaxdev.mac.m68k_sweep import is_020_only

md = Cs(CS_ARCH_M68K, CS_MODE_M68K_040); md.detail = True
NAMES = {}
CODE = {}
RELOC = {}
R = {}
JT = []
SEGNAME = {}

def trapname(w):
    if w in NAMES: return NAMES[w]
    if w & 0x0800:
        k = 0xA800 | (w & 0x3FF)
    else:
        k = 0xA000 | (w & 0xFF)
    n = NAMES.get(k, '?')
    return n + ('(autopop)' if (w & 0x0800 and w & 0x0400) else '(flags)')

def load(path):
    global R, SEGNAME, CODE, RELOC, JT
    rs = read_resource_fork(Path(path))
    R = {(r.kind, r.rid): r for r in rs}
    SEGNAME = {r.rid: r.name for r in rs if r.kind == b'CODE'}
    CODE = {r.rid: r.body for r in rs if r.kind == b'CODE' and r.rid != 0}
    if any(r.kind == b'CREL' for r in rs):
        raise ValueError('Unexpected CREL')
    RELOC = {n: {} for n in CODE}
    code0 = R[(b'CODE', 0)].body
    above, below, length, offset = struct.unpack_from('>IIII', code0)
    if (above, below, length, offset) != (8176, 22266, 8144, 32) or len(code0) != 8160:
        raise ValueError('CODE 0 / unsupported Prince 1.0 layout')
    JT = []
    for i in range(16, len(code0), 8):
        off, push, seg, trap = struct.unpack_from('>HHHH', code0, i)
        if (push != 0x3f3c or trap != 0xa9f0 or seg not in CODE
                or off & 1 or off + 4 >= len(CODE[seg])):
            raise ValueError(f'unsupported CODE 0 entry at {i:#x}')
        JT.append((seg, off + 4))

def jt_index(a5off):
    if 34 <= a5off < 34 + 8 * len(JT) and (a5off - 34) % 8 == 0:
        return (a5off - 34) // 8
    return None

def seg_label(n): return f"{n}:{SEGNAME.get(n,'') or 'CODE1'}"

STOP = {"rts", "rte", "rtr", "bra", "illegal", "jmp", "rtd"}
BR = re.compile(r"^(b(ra|sr|hi|ls|cc|cs|ne|eq|vc|vs|pl|mi|ge|lt|gt|le)|db[a-z]{1,3}|jsr|jmp)$")

def ops(i):
    try: return i.operands
    except CsError: return []

class Walker:
    def __init__(self):
        self.seen = {n: set() for n in CODE}
        self.insn_at = {n: {} for n in CODE}
        self.traps = []        # (seg, pc, word, context list)
        self.lowmem = []       # (seg, pc, addr, text)
        self.absother = []
        self.blind = []        # (seg, pc, text)
        self.jtcalls = collections.Counter()
        self.codeptrs = collections.defaultdict(set)  # seg -> pc-relative lea/pea targets
        self.jt_taken = collections.Counter()          # jt entries whose address is taken (not called)
        self.i020 = collections.Counter()
        self.i020_kind = collections.defaultdict(collections.Counter)
        self.a5calls = []
        self.switches = []
        self.indirect_roots = set()
        self.pending = []
        self.ptr_roots = []
        self.jtsites = collections.defaultdict(list)
        self.intra = collections.defaultdict(list)

    def walk(self, seg, roots):
        code = CODE[seg]; rel = RELOC[seg]
        todo = list(roots)
        while todo:
            pc = todo.pop()
            ctx = []
            while 0 <= pc < len(code) - 1 and pc not in self.seen[seg]:
                w = struct.unpack_from('>H', code, pc)[0]
                if 0xA000 <= w <= 0xAFFF:
                    self.seen[seg].update((pc, pc + 1))
                    self.traps.append((seg, pc, w, list(ctx[-10:])))
                    ctx.append(f"_{trapname(w)}")
                    pc += 2
                    if (w & 0x0C00) == 0x0C00 or w in (0xA9F4, 0xA9C9):   # autopop / ExitToShell / SysError
                        break
                    continue
                try:
                    insn = next(iter(md.disasm(code[pc:pc + 16], pc, count=1)), None)
                except CsError:
                    insn = None
                if insn is None or insn.size == 0 or insn.mnemonic == 'dc.w':
                    self.blind.append((seg, pc, 'undecodable'))
                    break
                for k in range(insn.size): self.seen[seg].add(pc + k)
                self.insn_at[seg][pc] = insn
                mn = insn.mnemonic.lower(); base = mn.split('.')[0]
                why = is_020_only(insn)
                if why:
                    self.i020[seg] += 1
                    self.i020_kind[seg][why.split(' ')[0] if 'scaled' not in why and 'full' not in why else why.split(' (')[0][:24]] += 1
                txt = f"{mn} {insn.op_str}"
                relocs, a5targets = [], []
                ctx.append(txt)
                # low memory
                for op in ops(insn):
                    if op.type == M68K_OP_MEM and op.address_mode in (M68K_AM_ABSOLUTE_DATA_SHORT, M68K_AM_ABSOLUTE_DATA_LONG):
                        a = op.imm & 0xffffffff
                        if not relocs:
                            if a < 0x2000 or a >= 0xFFFF8000:
                                self.lowmem.append((seg, pc, a, txt))
                            else:
                                self.absother.append((seg, pc, a, txt))
                    if op.type == M68K_OP_MEM and op.address_mode == M68K_AM_PCI_DISP and base in ('lea', 'pea'):
                        self.codeptrs[seg].add(pc + 2 + op.mem.disp)
                # control flow
                if base in ('jsr', 'jmp'):
                    tgt_jt = None
                    if a5targets and insn.size == 6:
                        tgt_jt = jt_index(a5targets[0])
                    m = re.match(r'^\$([0-9a-f]+)\(a5\)$', insn.op_str)
                    if m:
                        tgt_jt = jt_index(int(m.group(1), 16))
                    if tgt_jt is not None:
                        self.jtcalls[tgt_jt] += 1
                        self.jtsites[tgt_jt].append((seg, pc))
                        self.pending.append(tgt_jt)
                        ctx[-1] += f"  -> JT{tgt_jt} {seg_label(JT[tgt_jt][0])}+{JT[tgt_jt][1]:#06x}"
                        if base == 'jmp': break
                        pc += insn.size; continue
                    t = self.target(insn)
                    sw = self.switch_targets(seg, code, pc, insn) if base == 'jmp' and t is None else None
                    if sw:
                        self.switches.append((seg, pc, len(sw)))
                        self.indirect_roots.update((seg,x) for x in sw)
                        todo.extend(x for x in sw if x not in self.seen[seg])
                    elif t is not None:
                        if base == 'jsr': self.intra[(seg, t)].append(pc)
                        if 0 <= t < len(code) and t not in self.seen[seg]: todo.append(t)
                    else:
                        if not a5targets:
                            self.blind.append((seg, pc, txt))
                    if base == 'jmp': break
                    pc += insn.size; continue
                mm = re.search(r'\$([0-9a-f]+)\(a5\)', insn.op_str)
                if mm and base in ('pea', 'lea') and jt_index(int(mm.group(1), 16)) is not None:
                    j = jt_index(int(mm.group(1), 16))
                    self.jt_taken[j] += 1; self.pending.append(j)
                    ctx[-1] += f"  (&JT{j} {seg_label(JT[j][0])}+{JT[j][1]:#06x})"
                if a5targets and base in ('pea', 'move', 'lea', 'movea'):
                    j = jt_index(a5targets[0])
                    if j is not None:
                        self.jt_taken[j] += 1
                        self.pending.append(j)
                        ctx[-1] += f"  (&JT{j} {seg_label(JT[j][0])}+{JT[j][1]:#06x})"
                if BR.match(base):
                    t = self.target(insn)
                    if base == 'bsr' and t is not None: self.intra[(seg, t)].append(pc)
                    if t is not None and 0 <= t < len(code):
                        if t not in self.seen[seg]: todo.append(t)
                    else:
                        self.blind.append((seg, pc, txt))
                if base=='bsr':
                    target=self.target(insn)
                    tail=list(md.disasm(code[target:target+4],target)) if target is not None and 0<=target<len(code)-3 else []
                    if len(tail)==2 and tail[0].mnemonic.startswith('movea') and tail[0].op_str.startswith('(a7)+, a') and tail[1].mnemonic=='rts':
                        break  # return-address getter; inline slot is data, not fall-through
                if base in STOP: break
                pc += insn.size

    def switch_targets(self, seg, code, pc, insn):
        # idiom: cmpi.w #N,Dn ; bhi.w dflt ; lea T(pc),Ax ; adda.w (Ax,Dn.w*2),Ax ; jmp (Ax)
        # THINK near-model switch: bounded index, doubled, signed table offset.
        prior = [self.insn_at[seg][a] for a in sorted(self.insn_at[seg]) if pc-24 <= a < pc]
        if len(prior)>=5 and insn.op_str.endswith('(pc,d0.w)'):
            q=prior[-5:]
            bound=re.fullmatch(r'#\$([0-9a-f]+), d1',q[0].op_str)
            table=re.fullmatch(r'\$([0-9a-f]+)\(pc, d0.w\), d0',q[4].op_str)
            if (bound and table and q[0].mnemonic=='moveq' and
                q[1].mnemonic=='cmp.w' and q[1].op_str=='d1, d0' and
                q[2].mnemonic.startswith('bgt') and q[3].mnemonic=='add.w' and
                q[3].op_str=='d0, d0' and q[4].mnemonic=='move.w'):
                n=int(bound[1],16)+1; tbl=int(table[1],16)
                if not 0<n<=128 or tbl+2*n>len(code):raise ValueError('switch extent')
                targets=[pc+2+struct.unpack_from('>h',code,tbl+2*k)[0] for k in range(n)]
                if any(t&1 or not 4<=t<len(code)-1 for t in targets):raise ValueError('switch target')
                self.seen[seg].update(range(tbl,tbl+2*n))
                return targets
        # StripAddress trampoline: LEA target(PC),A0; MOVE A0,D0;
        # _StripAddress; MOVEA D0,A0; JMP (A0). No guessed register targets.
        if insn.op_str=='(a0)' and pc>=10 and code[pc-6:pc-2]==bytes.fromhex('2008a055') and code[pc-2:pc]==bytes.fromhex('2040'):
            lea=self.insn_at[seg].get(pc-10)
            if lea and lea.mnemonic.startswith('lea') and lea.op_str.endswith(', a0'):
                target=self.target(lea)
                if target is not None:return [target]
        m = re.match(r'^\(a(\d)\)$', insn.op_str)
        if not m: return None
        ax = m.group(1)
        # scan back up to 5 instructions via insn_at
        addrs = sorted(a for a in self.insn_at[seg] if pc - 24 <= a < pc)
        seq = [self.insn_at[seg][a] for a in addrs]
        if len(seq) < 2: return None
        adda, lea = seq[-1], seq[-2]
        if not (adda.mnemonic.startswith('adda.w') and re.match(rf'^\(a{ax}, d(\d)\.w \* 2\), a{ax}$', adda.op_str)):
            return None
        if not (lea.mnemonic.startswith('lea') and 'pc' in lea.op_str):
            return None
        tbl = self.target(lea)
        n = None
        for q in reversed(seq[:-2]):
            mm = re.match(r'^#\$([0-9a-f]+), d\d$', q.op_str)
            if q.mnemonic.startswith('cmp') and mm:
                n = int(mm.group(1), 16) + 1; break
        if n is None or n > 512: return None
        if tbl is None or tbl < 0 or tbl + 2 * n > len(code):
            raise ValueError(f'invalid switch table at {seg}:{pc:#x}')
        out = []
        for k in range(n):
            d = struct.unpack_from('>h', code, tbl + 2 * k)[0]
            target = tbl + d
            if target & 1 or not 0 <= target < len(code) - 1:
                raise ValueError(f'invalid switch target at {seg}:{pc:#x}')
            out.append(target)
        self.seen[seg].update(range(tbl, tbl + 2 * n))
        return out

    @staticmethod
    def target(insn):
        for op in ops(insn):
            if op.type == M68K_OP_BR_DISP:
                return insn.address + 2 + op.br_disp.disp
            if op.type == M68K_OP_MEM and op.address_mode == M68K_AM_PCI_DISP:
                return insn.address + 2 + op.mem.disp
        return None

def run(extra=False):
    W = Walker()
    per = collections.defaultdict(list)
    for s, o in JT: per[s].append(o)
    for s in sorted(CODE):
        W.walk(s, per.get(s, []))
    if extra:
        # second pass: pc-relative LEA/PEA targets that look like code, iterate
        for _ in range(5):
            added = False
            for s in sorted(CODE):
                new = [t for t in W.codeptrs[s] if t not in W.seen[s] and 0 <= t < len(CODE[s]) - 1
                       and CODE[s][t:t+2] in (b'\x4e\x56', b'\x48\xe7', b'\x2f\x0b', b'\x2f\x0a')]
                if new:
                    added = True; W.walk(s, new)
            if not added: break
    return W
