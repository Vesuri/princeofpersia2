#!/usr/bin/env python3
"""Run the helper on A1200/KS3.1 with a 4096-byte stack on a 68030; stop on completion."""
import hashlib
import os
from pathlib import Path
import socket
import shutil
import subprocess
import sys
import tempfile
import time

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(Path(__file__).resolve().parent))
from test_install import EXPECTED, SOURCE

def main():
    build=ROOT/"build/install-data-stack"
    subprocess.run(["make","-C",str(ROOT/"tools/install-data"),"amiga","BUILD=../../build/install-data-stack","EXTRA_AMIGA_FLAGS=-DINSTALL_STACK_TEST"],check=True)
    with tempfile.TemporaryDirectory(prefix="pop2-install-amiga-",dir=ROOT/"tmp") as tmp:
        base=Path(tmp); boot=base/"boot"; (boot/"s").mkdir(parents=True)
        (base/"state").mkdir(); (base/"home").mkdir()
        shutil.copyfile(build/"Pop2InstallData.exe",boot/"Extract")
        (boot/"scratch").mkdir()
        (base/"source").mkdir()
        shutil.copyfile(SOURCE,base/"source/original.sit")
        (boot/"s/startup-sequence").write_text('CD DH0:\nStack 4096\nExtract "DH1:original.sit" "DH2:installed" "DH0:scratch"\n')
        sock=socket.socket(); sock.bind(("127.0.0.1",0)); port=sock.getsockname()[1]; sock.close()
        commands=base/"test.gdb"
        commands.write_text(f'''set pagination off
set confirm off
set remotetimeout 90
target remote 127.0.0.1:{port}
break installer_test_done
continue
printf "INSTALLER result=%d stack=%u unused=%u\\n", installer_result, installer_stack_size, installer_stack_unused
if installer_result != 0 || installer_stack_size != 4096 || installer_stack_unused < 512
  quit 1
end
detach
quit
''')
        env=os.environ.copy(); env.update(HOME=str(base/"home"),XDG_CACHE_HOME=str(base/"home"))
        with (base/"fsuae.log").open("w") as log:
            launcher=base/"launch.sh"
            launcher.write_text('#!/bin/bash\nset -euo pipefail\ncd "$1"\n. "$2/amiga/env.sh"\nFSUAE_RUN=.\n. "$FSUAE_COMMON"\ntrap \'fsuae_stop\' EXIT\nFSUAE="$FSUAE_ARM"\nAMIGA_SETUP=a1200-030 DEBUG=1 DEBUG_PORT="$3" SOUND=0 WINDOW=none WARP=1\nfsuae_claim_port\nfsuae_options\nfsuae_launch --hard_drive_0="$PWD/boot" --hard_drive_1="$PWD/source" --hard_drive_2="$PWD" --remote_debugger_trigger=Extract --state_dir="$PWD/state" --floppy_drive_volume=0 --floppy_drive_volume_empty=0\nwait "$FSUAE_PID"\n')
            emu=subprocess.Popen(["/bin/bash",str(launcher),str(base),str(ROOT),str(port)],stdout=log,stderr=log)
            try:
                for _ in range(100):
                    if emu.poll() is not None: raise RuntimeError("FS-UAE exited: "+(base/"fsuae.log").read_text()[-2000:])
                    # A listening-port check does not consume a GDB connection.
                    check=subprocess.run(["lsof","-nP",f"-iTCP:{port}","-sTCP:LISTEN"],capture_output=True)
                    if check.returncode==0: break
                    time.sleep(.2)
                logfile=ROOT/"tmp/installer-amiga.log"
                with logfile.open("w") as debuglog:
                    result=subprocess.run(["m68k-amiga-elf-gdb","-q","-batch","-x",str(commands),str(build/"Pop2InstallData.elf")],env=env,stdout=debuglog,stderr=debuglog,timeout=900)
                report=logfile.read_text(); print("\n".join(line for line in report.splitlines() if "INSTALLER " in line or "Exception" in line))
                assert result.returncode==0 and "INSTALLER result=0" in report
                for name,digest in EXPECTED.items():
                    assert hashlib.sha256((base/"installed"/name).read_bytes()).hexdigest()==digest
                assert not list((base/"installed").glob(".pop2-install-*"))
                assert not list((base/"installed").glob(".pop2-publish-*"))
                assert not list((boot/"scratch").iterdir())
                print("PASS: Amiga extraction, exact hashes, 4 KiB stack and cleanup")
            finally:
                subprocess.run(["/bin/bash","-c",'. "$1/amiga/env.sh"; FSUAE_RUN="$2"; . "$FSUAE_COMMON"; fsuae_stop_previous',"stop",str(ROOT),str(base)],check=True)
                try: emu.wait(timeout=5)
                except subprocess.TimeoutExpired: emu.kill(); emu.wait()

if __name__=="__main__": main()
