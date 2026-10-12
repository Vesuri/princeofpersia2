"""Pin the original allocation/release fixture fields consumed by the host probe."""
import hashlib,json,struct
from audio_inputs import content
from extract_original_data import ROOT,pins

def contract(directory):
    log=(directory/'session.log').read_text()
    assert log.count('AUDIO_FIXTURE_COMPLETE calls=27\n')==1 and 'FAIL REFERENCE' not in log,'AUDIO / ALLOCATION COMPLETION'
    assert (directory/'audio-driver.bin').read_bytes()==content()['driver-expanded.bin'],'AUDIO / ALLOCATION DRIVER'
    def data(side,index):
        b=(directory/f'audio-fixture-{side}-{index}.bin').read_bytes();assert len(b)==0x32f0;return b
    word=lambda b,at:int.from_bytes(b[at:at+2],'big')
    long=lambda b,at:int.from_bytes(b[at:at+4],'big')
    first=data('before',1)
    result={'flags':word(first,0x13d0),'counts':[word(first,at) for at in (0x13d6,0x13d8,0x13da)],'allocation':[]}
    for i in range(1,14):
        for side in ('before','after'):
            b=data(side,i);result['allocation'].append([[word(b,0x24f2+4*slot+offset) for offset in (0x200,0x480,0x4c0,0x440,0x400)] for slot in range(5)])
    b=data('after',15);base=long(b,0x24f2)
    result['sample']=[long(b,0x24f2+offset)-base if long(b,0x24f2+offset) else 0 for offset in (0x280,0x2c0,0x300)]
    result['step']=long(b,0x2532);result['release']=[]
    for i in range(16,28):
        b=data('after',i);result['release'].append([word(b,0x26f2),word(b,0x29f2),long(b,0x24f2)-base])
    return (json.dumps(result,sort_keys=True,separators=(',',':'))+'\n').encode()

def verify(directory):
    digest=hashlib.sha256(contract(directory)).hexdigest()
    assert digest==pins(ROOT/'manifest/audio-reference.sha256')['allocation-contract.json'],'AUDIO / ALLOCATION PIN'
    return digest
