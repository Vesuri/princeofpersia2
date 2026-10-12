"""Build an ignored catalog of original names, sizes and pinned fork hashes."""
import hashlib
import struct
from extract_original_data import ROOT, DESTINATION, pins, verify_output


def catalog():
    verify_output(DESTINATION)
    hashes=pins(ROOT/'manifest/runtime-data.sha256')
    names=sorted(name[:-6] for name in hashes if name.endswith('.finfo'))
    output=bytearray(b'PFC1'+struct.pack('>I',len(names)))
    for name in names:
        encoded=name.encode('ascii')
        output+=struct.pack('>H',len(encoded))+encoded
        for suffix in ('','.rsrc'):
            output+=struct.pack('>I',(DESTINATION/(name+suffix)).stat().st_size)
            output+=bytes.fromhex(hashes[name+suffix])
        output+=bytes.fromhex(hashes[name+'.finfo'])
    return bytes(output)


if __name__=='__main__':
    data=catalog();(ROOT/'build/core/file-catalog.bin').write_bytes(data)
    print(hashlib.sha256(data).hexdigest()+'  file-catalog.bin')
