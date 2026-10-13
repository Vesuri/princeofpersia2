"""Synthetic inherited helper fixtures; no original game inputs."""
from pathlib import Path
import os,struct
import subprocess
ROOT=Path(__file__).resolve().parents[1]
os.chdir(ROOT)
out=ROOT/'build/host';out.mkdir(parents=True,exist_ok=True)
compiler=os.environ.get('CXX','c++')
sources=sorted(str(p) for p in Path('src/mac').glob('*.cpp'))
for test in sorted(Path('tools').glob('test_*.cpp')):
    exe=out/test.stem
    subprocess.run([compiler,'-std=c++17','-O2','-DAMIGAXDEV_RESOURCE_DIRECTORY_INDEXED','-DAMIGAXDEV_RESOURCE_DIRECTORY_MAP_LIMIT','-include','cstdint','-Isrc',str(test),*sources,'-o',str(exe)],check=True)
    if test.stem in {'test_fillrect8','test_line8'}:
        rect=struct.pack('>hhhh',0,0,480,640)
        shape=struct.pack('>hhhh',20,30,25,36)
        data=rect*4+shape+bytes(640*480)
        args=[str(exe)]+(['--synthetic'] if test.stem=='test_line8' else [])
        result=subprocess.run(args,input=data,stdout=subprocess.PIPE,check=True).stdout
        assert len(result)==640*480
        if test.stem=='test_fillrect8':
            expected=bytearray(640*480)
            for y in range(20,25):expected[y*640+30:y*640+36]=bytes([255])*6
            assert result==expected
        else:assert any(result)
        print('PASS synthetic '+test.stem)
    else:subprocess.run([str(exe)],check=True)
subprocess.run([compiler,'-std=c++17','src/main.cpp','-o',str(out/'PrinceOfPersia2')],check=True)
subprocess.run([str(out/'PrinceOfPersia2')],check=True)
print('HOST SCAFFOLD PASS')
