"""Check the shared-reader pin rejects an otherwise valid altered module."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
from amigaxdev import aprof

ROOT=Path(__file__).resolve().parents[1]

def check(run):
    command=[sys.executable,str(ROOT/'tools/aprof_report.py'),'check-run',str(run.resolve())]
    positive=subprocess.run(command,cwd=ROOT,capture_output=True,text=True)
    assert positive.returncode==0 and 'OPENING RUN PASS' in positive.stdout,positive.stderr
    with tempfile.TemporaryDirectory(prefix='profile-pin-',dir=ROOT/'tmp') as work:
        package=Path(work)/'amigaxdev';package.mkdir()
        (package/'__init__.py').write_text('')
        module=package/'aprof.py'
        module.write_bytes(Path(aprof.__file__).read_bytes())
        env=dict(os.environ,PYTHONPATH=work)
        control=subprocess.run(command,cwd=ROOT,env=env,capture_output=True,text=True)
        assert control.returncode==0 and 'OPENING RUN PASS' in control.stdout,control.stderr
        module.write_bytes(module.read_bytes()+b'\n# altered shared source\n')
        rejected=subprocess.run(command,cwd=ROOT,env=env,capture_output=True,text=True)
        assert rejected.returncode!=0 and 'SHARED PROFILE READER PIN' in rejected.stderr,rejected.stderr
        assert 'OPENING RUN PASS' not in rejected.stdout
    print('PASS shared reader: original and relocated source accepted; altered source rejected before capture use')

if __name__=='__main__':check(Path(sys.argv[1]))
