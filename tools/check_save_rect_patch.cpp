// Host rejection controls execute the same guard and patch as the game loader.
#include "../src/platform/amiga/SaveRectPatch.h"
#include <cassert>
#include <fstream>
#include <vector>
int main(int argc,char** argv){
 assert(argc==2);std::ifstream in(argv[1],std::ios::binary);
 std::vector<uint8_t> oracle((std::istreambuf_iterator<char>(in)),{});
 assert(oracle.size()==36);
 std::vector<uint8_t> original(0x5400,0x5a);
 for(unsigned i=0;i<34;++i)original[0x53a4+i]=oracle[i];
 auto patched=original;assert(SaveRectPatch::install(patched.data(),patched.size(),0x123456));
 for(unsigned i=0;i<patched.size();++i)if(i<0x53a4||i>=0x53ae)assert(patched[i]==original[i]);
 assert(BigEndianMemory::read16(patched.data()+0x53a4)==0x4eb9);
 assert(BigEndianMemory::read32(patched.data()+0x53a6)==0x123456);
 assert(BigEndianMemory::read16(patched.data()+0x53aa)==0x6000);
 assert(0x53ac+BigEndianMemory::read16(patched.data()+0x53ac)==0x53c6);
 for(unsigned i=0;i<34;++i)for(unsigned bit=0;bit<8;++bit){auto bad=original;bad[0x53a4+i]^=1<<bit;auto before=bad;assert(!SaveRectPatch::install(bad.data(),bad.size(),0x123456));assert(bad==before);}
 for(unsigned size:{0U,0x53a4U,0x53c5U}){auto bad=original;assert(!SaveRectPatch::install(bad.data(),size,0x123456));assert(bad==original);}
 for(unsigned entry:{0U,1U,0x123457U}){auto bad=original;assert(!SaveRectPatch::install(bad.data(),bad.size(),entry));assert(bad==original);}
}
