"""Check the paired VBL fixture; this does not establish game-step cadence."""
import argparse
import hashlib
import re
import struct
from pathlib import Path
from check_core_result import restored
from extract_original_data import ROOT, pins


def require(ok, reason):
    if not ok:
        raise RuntimeError("TIMING VBL / " + reason)


def original(reference):
    path = reference / "session.log"
    expected = pins(ROOT / "manifest/timing-vbl-reference.sha256")
    require(set(expected) == {"vbl-contract.txt"}, "REFERENCE PIN COVERAGE")
    data = path.read_bytes()
    log = data.decode()
    require(log.count("TIMING_VBL_COMPLETE removal=1\n") == 1, "ORIGINAL COMPLETION")
    require("FAIL REFERENCE" not in log and "[LUA ERROR]" not in log, "ORIGINAL FAILURE")
    rows = re.findall(r"^TIMING_VBL initial=([0-9A-F]{4}) calls=(\d+) error=([0-9A-F]{4}) count=([0-9A-F]{4}) link=[0-9A-F]{8} last=([0-9A-F]{8})$", log, re.M)
    require(len(rows) == 4, "ORIGINAL TASK RECORDS")
    starts = re.findall(r"^TIMING_VBL_START initial=([0-9A-F]{4}) count=([0-9A-F]{4})$", log, re.M)
    if starts:
        require(len(starts)==4 and [r[0] for r in starts]==[r[0] for r in rows], 'ORIGINAL STABLE SAMPLES')
        normalized=[]
        for i,(a,b,c,d,e) in enumerate(rows):
            if i>=2:
                require(((int(starts[i][1],16)-int(d,16))&0xffff)==30, 'ORIGINAL OBSERVED COUNTDOWN')
                d=f'{(int(a,16)-30)&0xffff:04X}'
            normalized.append((a,b,c,d,e))
        rows=normalized
    require([(int(a,16),int(b),int(c,16),int(d,16)) for a,b,c,d,e in rows] ==
            [(1,3,0,0),(2,3,0,0),(0,0,0,0xffe2),(0xffff,0,0,0xffe1)], "ORIGINAL COUNTDOWN CONTRACT")
    require(re.findall(r"^TIMING_VREMOVE initial=([0-9A-F]{4}) error=([0-9A-F]{4})$", log, re.M) ==
            [("0001","FFFF"),("0002","FFFF"),("0000","0000"),("FFFF","0000")], "ORIGINAL REMOVAL CONTRACT")
    require(log.count("TIMING_VBL_OBSERVED elapsed=30\n") == 1, "ORIGINAL OBSERVATION CLOCK")
    # Pin the observed service contract, excluding relocated pointers, absolute
    # clock epochs and unrelated emulator output so fresh references can repeat.
    removals = re.findall(r"^TIMING_VREMOVE initial=([0-9A-F]{4}) error=([0-9A-F]{4})$", log, re.M)
    contract = "elapsed=30\n" + "".join(
        f"initial={a} calls={b} install={c} count={d} remove={removed}\n"
        for (a,b,c,d,last), (_,removed) in zip(rows,removals))
    require(hashlib.sha256(contract.encode()).hexdigest() == expected["vbl-contract.txt"], "REFERENCE PIN")
    require(all(int(row[4],16) > 0 for row in rows[:2]), "ORIGINAL CALLBACK CLOCK")



def records(data, stable=False):
    require(len(data) == 160, "RECORD EXTENT")
    rows = [struct.unpack(">IHIHHHHHIHHI", data[i:i+32]) for i in range(0,160,32)]
    callbacks = {r[2] for r in rows}
    require(len(callbacks) == 1 and 0 not in callbacks, "CALLBACK IDENTITY")
    for i, row in enumerate(rows):
        link, kind, callback, count, phase, unused, calls, installed, last, observed, removed, first = row
        require(kind == 1 and phase == 0 and installed == 0, "INSTALL " + str(i))
        if i in (0,1,4):
            require(calls == 3 and count == observed == 0 and removed == 0xffff, "REARM AND AUTO-REMOVE " + str(i))
            require(first != 0 and ((last-first)&0xffffffff) == 2, "CONSECUTIVE CALLBACK TICKS " + str(i))
        else:
            require(calls == 0 and first == last == 0 and removed == 0, "DORMANT TASK " + str(i))
            expected=((unused-30)&0xffff) if stable else (0xffe2 if i == 2 else 0xffe1)
            require(observed == expected, "DORMANT COUNTDOWN " + str(i))
    return rows


def evidence(log, data):
    require(log.count("CLOCK FAULT 0\n")==1, "MONOTONIC CLOCK")
    require(log.count("CORE REASON TIMING / VBL FIXTURE COMPLETE\n") == 1, "POSITIVE COMPLETION")
    require("CORE EXCEPTION vector=0 pc=00000000\n" in log, "EXCEPTION")
    require("INTEGRATION AUDIT code=1 input=1\n" in log, "CODE AND INPUT AUDITS")
    require(log.count("TIMING VBL RESTORED\n") == 1, "RESTORED MILESTONE")
    require(re.findall(r"^TIMING VBL result=(\d+) elapsed=(\d+) user_calls=(\d+) deferred_fields=(\d+)$", log, re.M) ==
            [("1","30","9","6")], "ABI, USER MODE AND DEFERRED DISPATCH")
    records(data, stable=log.count('TIMING VBL STABLE_SAMPLES\n')==1)


def check(run, reference, setup, controls=False):
    original(reference)
    log = (run / "gdb-out.log").read_text()
    data = (run / "timing-vbl.bin").read_bytes()
    restored(run, log, setup)
    evidence(log, data)
    if controls:
        changed = []
        for old, new in [("TIMING / VBL FIXTURE COMPLETE", "TIMING / TIMEOUT"),
                         ("TIMING VBL RESTORED", "NOT RESTORED"),
                         ("user_calls=9", "user_calls=8"),
                         ("deferred_fields=6", "deferred_fields=0"),
                         ("result=1", "result=0"),
                         ("CLOCK FAULT 0", "CLOCK FAULT 1")]:
            require(old in log, "INACTIVE LOG CONTROL")
            changed.append((log.replace(old,new),data))
        for offset, value in [(16,0),(26,0),(2*32+24,0),(4*32+16,2)]:
            bad = bytearray(data)
            struct.pack_into(">H",bad,offset,value)
            require(bad != data, "INACTIVE RECORD CONTROL")
            changed.append((log,bytes(bad)))
        if 'TIMING VBL STABLE_SAMPLES\n' in log:
            bad=bytearray(data);bad[2*32+14]^=1
            changed.append((log,bytes(bad)))
        bad = bytearray(data)
        struct.pack_into(">I",bad,4*32+20,struct.unpack_from(">I",bad,4*32+28)[0]+1)
        changed.extend([(log,bytes(bad)),(log,data[:-1])])
        for bad_log, bad_data in changed:
            try:
                evidence(bad_log,bad_data)
            except RuntimeError:
                continue
            raise RuntimeError("TIMING VBL / ACCEPTED ALTERED EVIDENCE")
        evidence(log,data)
        print(f"TIMING VBL CONTROLS PASS: {len(changed)} alterations rejected; restored positive passed")
    print("TIMING VBL PASS: original countdown/removal contract, user-mode ABI and six-field deferred dispatch; not game cadence")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("run", type=Path)
    parser.add_argument("reference", type=Path)
    parser.add_argument("--setup", choices=["a1200-fast", "a1200-030", "a1200-040"], required=True)
    parser.add_argument("--controls", action="store_true")
    args = parser.parse_args()
    check(args.run, args.reference, args.setup, args.controls)
