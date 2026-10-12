"""Host model of CODE 11 Decrypt/DecryptHandle and LZSSUncompress."""
import struct

def decrypt(raw):
    out=bytearray(raw);state=0xdce5
    for i in range(7,len(raw)):
        cipher=raw[i];out[i]=cipher^(state>>8)
        state=((cipher+state)*0xce6d+0x58bf)&0xffff
    return bytes(out)

def expand(raw):
    size=struct.unpack_from('>I',raw)[0]
    if size>1000000:raise ValueError('MDRV size')
    out=bytearray();i=4
    while i<len(raw) and len(out)<size:
        flags=raw[i];i+=1
        for bit in range(8):
            if i>=len(raw) or len(out)>=size:break
            if flags&(1<<bit):
                out.append(raw[i]);i+=1
            else:
                if i+2>len(raw):raise ValueError('MDRV truncated back reference')
                word=int.from_bytes(raw[i:i+2],'big');i+=2
                offset=(word&4095)-4096;count=(word>>12)+3
                if len(out)+offset<0 or len(out)+count>size:raise ValueError('MDRV back reference extent')
                for _ in range(count):out.append(out[len(out)+offset])
    if i!=len(raw) or len(out)!=size:raise ValueError('MDRV stream extent')
    return bytes(out)
