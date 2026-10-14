"""Generate byte-verified owner-authorized UI seams; original bytes stay ignored."""
from pathlib import Path
import hashlib,struct
from extract_original_data import ROOT,DESTINATION,verify_output,pins
import trap_census as census
# Authored replacements. All branch destinations are original CODE offsets.
# D6 retains the original successful menu updates and records acceptance.
# D9 returns false from both packing offers; D8 omits Macintosh menu-bar drawing.
SEAMS=[
 (2,0x1a40,struct.pack('>6H',0x7001,0x3b40,(-0x56d8)&65535,0x6000,0x1b8c-0x1a48,0x4e71),'CopyProtectionAccepted'),
 (17,0x4560,struct.pack('>2H',0x7000,0x4e75),'NoStartupCompression'),
 (17,0x463a,struct.pack('>2H',0x7000,0x4e75),'NoGameCompression'),
 (2,0x3ae8,struct.pack('>2H',0x4e75,0x4e71),'NoMacMenuHide'),
 (2,0x3b76,struct.pack('>2H',0x4e75,0x4e71),'NoMacMenuShow'),
 (17,0x245e,struct.pack('>3H',0xaffd,1,0x4e75),'PortAbout'),
 (17,0x6d42,struct.pack('>4H',0xaffd,2,0x4e75,0x4e71),'PortAlert'),
 (2,0x2220,struct.pack('>3H',0xaffd,3,0x4e75),'PortHallName'),
]
def content():
 verify_output(DESTINATION);census.load(DESTINATION/'Prince of Persia 2.rsrc')
 rows=[]
 for segment,offset,replacement,name in SEAMS:
  original=census.CODE[segment][offset:offset+len(replacement)]
  rows.append(struct.pack('>HHI',segment,len(original),offset)+original.ljust(12,b'\0')+replacement.ljust(12,b'\0'))
 return struct.pack('>I',len(rows))+b''.join(rows)
def generate(verify=False):
 data=content();digest=hashlib.sha256(data).hexdigest()
 expected=pins(ROOT/'manifest/ui-inputs.sha256')['ui-seams.bin']
 if digest!=expected:raise ValueError('UI / SEAM INPUT PIN')
 header='// Generated hashes only. Original byte guards stay in the ignored input.\nstatic const uint8_t uiSeamsSHA256[32]={'+','.join('0x'+digest[i:i+2] for i in range(0,64,2))+'};\n'
 out=ROOT/'build/ui'
 if verify:
  if (out/'ui-seams.bin').read_bytes()!=data or (out/'UIInputs.h').read_text()!=header:raise ValueError('UI / GENERATED INPUT')
 else:
  out.mkdir(parents=True,exist_ok=True);(out/'ui-seams.bin').write_bytes(data);(out/'UIInputs.h').write_text(header)
 print('UI INPUTS PASS:',len(SEAMS),'byte-verified owner-authorized seams')
if __name__=='__main__':
 import sys
 generate('--verify' in sys.argv)
