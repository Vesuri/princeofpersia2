"""Compare shared one-bit bitmap region encoding with original live calls."""
import argparse,hashlib,re,subprocess
from pathlib import Path
from extract_original_data import ROOT,pins

def check(run):
    for name,digest in pins(ROOT/'manifest/gameplay-bitmap-region.sha256').items():
        assert hashlib.sha256((run/name).read_bytes()).hexdigest()==digest,'BITMAP REGION / HASH'
    text=(run/'session.log').read_text()
    assert text.count('GAMEPLAY_ENDING_COMPLETE\n')==1 and 'FAIL REFERENCE' not in text,'BITMAP REGION / COMPLETION'
    records=re.findall(r'^GAMEPLAY_BITMAP_RETURN id=(\d+) size=(\d+)$',text,re.M)
    assert records and [int(r[0]) for r in records]==list(range(1,len(records)+1)),'BITMAP REGION / CALLS'
    output=ROOT/'build/host/check_bitmap_region';output.parent.mkdir(parents=True,exist_ok=True)
    subprocess.run(['clang++','-std=c++17','-fsanitize=address,undefined','-I',str(ROOT/'src'),str(ROOT/'tools/check_bitmap_region.cpp'),'-o',str(output)],check=True)
    for index,size in records:
        prefix=run/f'bitmap-{index}'
        assert Path(str(prefix)+'-region.bin').stat().st_size==int(size),'BITMAP REGION / SIZE'
        subprocess.run([str(output),*[str(prefix)+suffix for suffix in ('-map.bin','-pixels.bin','-region.bin')]],check=True)
    print(f'ORIGINAL BITMAP REGION PASS: {len(records)} complete byte-matched region encodings and extent controls')

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('run',type=Path)
    check(parser.parse_args().run)
