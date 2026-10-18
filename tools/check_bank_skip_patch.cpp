// Execute the production guard against pinned originals and the paired candidate.
#include "../src/platform/amiga/BankSkipPatch.h"
#include <cassert>
#include <fstream>
#include <vector>
#include <iostream>
static std::vector<uint8_t> read(const char* name) {
    std::ifstream in(name,std::ios::binary);
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(in)),{});
}
int main(int argc,char** argv) {
    assert(argc==3);auto oracle=read(argv[1]),candidate=read(argv[2]);
    assert(oracle.size()==178 && candidate.size()==178);
    std::vector<uint8_t> original(0x200,0x5a);
    for(unsigned i=0;i<176;++i)original[0x90+i]=oracle[i];
    auto patched=original;assert(BankSkipPatch::install(patched.data(),patched.size()));
    for(unsigned i=0;i<176;++i)assert(patched[0x90+i]==candidate[i]);
    for(unsigned i=0;i<patched.size();++i)
        if(i<0x10e||i>=0x116)assert(patched[i]==original[i]);
    unsigned rejected=0;
    for(unsigned i=0;i<176;++i)for(unsigned bit=0;bit<8;++bit) {
        auto bad=original;bad[0x90+i]^=1<<bit;auto before=bad;
        assert(!BankSkipPatch::install(bad.data(),bad.size()));assert(bad==before);++rejected;
    }
    for(unsigned size:{0U,0x90U,0x13fU}) {
        auto bad=original;assert(!BankSkipPatch::install(bad.data(),size));assert(bad==original);++rejected;
    }
    auto before=patched;assert(!BankSkipPatch::install(patched.data(),patched.size()));
    assert(patched==before);++rejected;
    std::cout<<"BANK PATCH PASS: exact paired bytes, unchanged surroundings, "<<rejected<<" rejections\n";
}
