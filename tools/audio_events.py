"""Build a host-only call replay and compare native decisions with original events."""
import re,struct,subprocess
from pathlib import Path
from extract_original_data import ROOT,DESTINATION,verify_output
import trap_census as census
from audio_inputs import content

from audio_replay import calls

def replay(directory):
    verify_output(DESTINATION);census.load(DESTINATION/'Prince of Persia 2.rsrc');events=calls(directory)
    data=bytearray(b'HAP1'+content()['audio-pitch.bin'])
    for i in range(34):
        inst=census.R[b'INST',i].body;sample=census.R[b'snd ',int.from_bytes(inst[:2],'big')].body
        data+=inst+struct.pack('>I',len(sample))+sample
    data+=struct.pack('>I',len(events))
    for row in events:
        data+=struct.pack('>8I',row['clock'],row['selector'],row['arg'],*struct.unpack('>5H',row['packet'][:10]))
    out=ROOT/'tmp/audio-analysis';out.mkdir(parents=True,exist_ok=True)
    (out/'events.bin').write_bytes(data)
    exe=out/'probe-events'
    subprocess.run(['c++','-std=c++17','-g','-fsanitize=address,undefined','-Isrc','tools/probe_audio_events.cpp','-o',str(exe)],cwd=ROOT,check=True)
    result=subprocess.run([str(exe)],input=data,stdout=subprocess.PIPE,check=True)
    actual=[tuple(map(int,line.split()[1:])) for line in result.stdout.splitlines() if line.startswith(b"N ")]
    releases=[tuple(map(int,line.split()[1:])) for line in result.stdout.splitlines() if line.startswith(b"R ")]
    expected_releases=[(i,slot,pos) for i,r in enumerate(events) for slot,pos in r["releases"]]
    if expected_releases and releases!=expected_releases:
        raise ValueError(("AUDIO RELEASE CURSOR",[(a,b) for a,b in zip(releases,expected_releases) if a!=b][:12],len(releases),len(expected_releases)))
    expected=[(i,r['selected'],r['step']) for i,r in enumerate(events) if r['selector']==9]
    differences=[(a,b) for a,b in zip(actual,expected) if a!=b]
    if len(actual)!=len(expected) or differences:raise ValueError(('AUDIO EVENT DIFFERENCE',len(actual),len(expected),differences[:10]))
    print('AUDIO MUSIC EVENT PAIR PASS:',len(actual),'note decisions and pitch steps')

if __name__=='__main__':
    import sys
    replay(Path(sys.argv[1]))
