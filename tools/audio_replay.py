"""Prepare a bounded, hash-verified original audio service replay (private data)."""
import hashlib,json,re,struct,sys
from pathlib import Path
from extract_original_data import ROOT,DESTINATION,verify_output,pins
from audio_inputs import content

OUT=ROOT/'build/audio'
def calls(directory):
    text=(directory/'session.log').read_text()
    if 'FAIL REFERENCE' in text or 'AUDIO_OBSERVER_COMPLETE' not in text or not any(x in text for x in ('AUDIO_ROUTE_COMPLETE','AUDIO_CUTSCENE_COMPLETE')):
        raise ValueError('AUDIO / INCOMPLETE ORIGINAL')
    if (directory/'audio-driver.bin').read_bytes()!=content()['driver-expanded.bin']:
        raise ValueError('AUDIO / ORIGINAL DRIVER IDENTITY')
    rows=[];stack=[];phase=0;effect=0;voices={};callbacks=0
    for line in text.splitlines():
        if line.startswith('AUDIO_PHASE '):
            phase={'opening-checkpoint':0,'title':1,'gameplay':2,'death':3}[line.split()[1]]
        elif line.startswith('AUDIO_CALL '):
            row={k:int(v,16) for k,v in re.findall(r'(\w+)=([0-9A-F]+)(?= |$)',line)}
            row['packet']=bytes.fromhex(re.search(r'packet=([0-9A-F/]+)',line)[1].replace('/',''))
            row.update(selected=-1,step=0,phase=phase,releases=[],callbacks=callbacks)
            if row['selector']==17:
                effect+=1;row['packet']=(directory/f'audio-packet-{effect}.bin').read_bytes()
                row['pcm']=(directory/f'audio-effect-{effect}.bin').read_bytes()
                assert len(row['packet'])==38 and len(row['pcm'])==int.from_bytes(row['packet'][4:8],'big')
            rows.append(row);stack.append(row)
        elif line.startswith('AUDIO_VOICE '):
            assert stack and stack[-1]['selector']==9
            stack[-1]['selected']=int(re.search(r'slot=(\w+)',line)[1],16)
            stack[-1]['step']=int(re.search(r'step=(\w+)',line)[1],16)
            voices[stack[-1]['selected']]=int(re.search(r'pcm=(\w+)',line)[1],16)
        elif line.startswith('AUDIO_RELEASE '):
            v={k:int(value,16) for k,value in re.findall(r'(\w+)=([0-9A-F]+)',line)}
            assert stack and stack[-1]['selector']==12 and v['slot'] in voices
            stack[-1]['releases'].append((v['slot'],v['cursor']-voices[v['slot']]))
        elif line.startswith('AUDIO_CALLBACK '):
            callbacks+=1
        elif line.startswith('AUDIO_RETURN '):
            values={k:int(v,16) for k,v in re.findall(r'(\w+)=([0-9A-F]+)',line)}
            assert stack,'AUDIO / UNPAIRED RETURN'
            row=stack.pop();assert (row['selector'],row['sp'])==(values['selector'],values['sp'])
            row['result']=values['d0'];row['callbacksAfter']=callbacks
    assert not stack and rows and effect,'AUDIO / MISSING EVENTS'
    return rows

def generate(directory):
    verify_output(DESTINATION);rows=calls(directory)
    sources={n:(DESTINATION/'Data'/n).read_bytes() for n in ('DigiSnd.dat','NISDIGI.dat')}
    records=bytearray();payload=bytearray();catalog=[]
    for r in rows:
        packet=bytearray(r['packet'].ljust(40,b'\0'));offset=size=0
        if r['selector']==17:
            matches=[(name,data.find(r['pcm'])) for name,data in sources.items() if r['pcm'] in data]
            assert len(matches)==1,'AUDIO / SAMPLE SOURCE'
            name,at=matches[0];offset=len(payload);size=len(r['pcm']);payload+=r['pcm']
            catalog.append({'file':name,'offset':at,'size':size,'sha256':hashlib.sha256(r['pcm']).hexdigest()})
            for p in (0,20,30,34):packet[p:p+4]=bytes(4)
        elif r['selector'] in (18,20):
            assert 'effectId' in r,'AUDIO / EFFECT ID NOT OBSERVED'
            packet=bytearray(40);struct.pack_into('>H',packet,24,r['effectId'])
            struct.pack_into('>II',packet,30,r['callback'],r['context'])
        elif r['selector']==27:
            assert not any(packet[:8]),'AUDIO / UNMEASURED MUSIC CALLBACK'
            packet=bytearray(40)
        records+=struct.pack('>4I',r['clock'],r['selector'],r['arg'],r['phase'])+packet+struct.pack('>2I',offset,size)
    data=b'PAR1'+struct.pack('>2I',len(rows),len(payload))+records+payload
    digest=hashlib.sha256(data).hexdigest()
    profile='route.bin' if any(r['phase'] for r in rows) else 'cutscene.bin'
    assert digest==pins(ROOT/'manifest/audio-replays.sha256')[profile],'AUDIO / REFERENCE REPLAY PIN'
    OUT.mkdir(parents=True,exist_ok=True)
    (OUT/'audio-replay.bin').write_bytes(data)
    (OUT/'AudioReplay.h').write_text('// Generated fixture hash only.\nstatic const uint8_t audioReplaySHA256[32]={'+','.join('0x'+digest[i:i+2] for i in range(0,64,2))+'};\n')
    (OUT/'audio-replay.json').write_text(json.dumps({'sha256':digest,'reference':str(directory.resolve().relative_to(ROOT)),'events':len(rows),'samples':catalog},indent=2)+'\n')
    print('AUDIO REPLAY INPUT:',len(rows),'calls;',len(catalog),'verified archive samples;',digest)

def verify():
    m=json.loads((OUT/'audio-replay.json').read_text());assert hashlib.sha256((OUT/'audio-replay.bin').read_bytes()).hexdigest()==m['sha256'],'AUDIO / REPLAY HASH'
    assert m['sha256'] in pins(ROOT/'manifest/audio-replays.sha256').values(),'AUDIO / REPLAY INPUT PIN'
    digest=m['sha256']
    expected='// Generated fixture hash only.\nstatic const uint8_t audioReplaySHA256[32]={'+','.join('0x'+digest[i:i+2] for i in range(0,64,2))+'};\n'
    assert (OUT/'AudioReplay.h').read_text()==expected,'AUDIO / REPLAY HEADER'
    return m
if __name__=='__main__':
    if sys.argv[1]=='--verify':verify()
    else:generate(Path(sys.argv[1]))
