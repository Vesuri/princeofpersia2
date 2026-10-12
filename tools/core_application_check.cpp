// Real-input preflight and corruption controls. Invoked separately from the
// synthetic host fixtures; no original bytes are compiled into this program.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <vector>
#include "../src/mac/OriginalApplication.h"
#include "../src/mac/SHA256.h"
static uint32_t lng(const uint8_t* p) { return uint32_t(p[0])<<24|uint32_t(p[1])<<16|uint32_t(p[2])<<8|p[3]; }
static void hash(const std::vector<uint8_t>& bytes,uint8_t* digest) {
    SHA256 sha;sha_init(&sha);sha_update(&sha,bytes.data(),bytes.size());sha_final(&sha,digest);
}
int main(int argc,char** argv) {
    assert(argc==3 && std::strlen(argv[2])==64);
    std::ifstream input(argv[1],std::ios::binary);assert(input);
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(input)),{});
    uint8_t expected[32];for(unsigned i=0;i<32;++i) {
        char pair[3]={argv[2][2*i],argv[2][2*i+1],0};char* end;
        expected[i]=std::strtoul(pair,&end,16);assert(*end==0);
    }
    OriginalApplication app;assert(app.open(bytes.data(),bytes.size(),expected));
    const auto w=app.world();
    std::printf("A5 above=%u below=%u jumpBytes=%u jumpOffset=%u\n",w.above,w.below,w.jumpBytes,w.jumpOffset);
    OriginalApplication::Resource code0,code1;
    assert(app.resource(0x434f4445,0,code0) && app.resource(0x434f4445,1,code1));
    auto codeOffset=code0.bytes-bytes.data();
    uint32_t start=lng(code0.bytes+12)+2;
    std::printf("ENTRY segment=1 offset=%u a5Offset=%u\n",unsigned(code0.bytes[16])*256+code0.bytes[17]+4,start);
    assert(!app.resource(0x434f4445,32767,code1) && code1.bytes==nullptr);
    bytes.back()^=1;
    assert(!app.open(bytes.data(),bytes.size(),expected) && !std::strcmp(app.error(),"APPLICATION / SHA256"));
    assert(!app.resource(0x434f4445,0,code0));
    bytes.back()^=1;
    assert(!app.open(bytes.data(),bytes.size()-1,expected));
    assert(!app.open(nullptr,bytes.size(),expected));
    assert(!app.open(bytes.data(),bytes.size(),nullptr));
    // Rehash deliberate format mutations to exercise the checks behind SHA256.
    auto changed=bytes;changed[codeOffset+8]^=1;uint8_t digest[32];hash(changed,digest);
    assert(!app.open(changed.data(),changed.size(),digest) && !std::strcmp(app.error(),"APPLICATION / A5 LAYOUT"));
    changed=bytes;changed[codeOffset+18]^=1;hash(changed,digest);
    assert(!app.open(changed.data(),changed.size(),digest) && !std::strcmp(app.error(),"APPLICATION / JUMP TABLE"));
    changed=bytes;changed[4]=0xff;hash(changed,digest);
    assert(!app.open(changed.data(),changed.size(),digest) && !std::strcmp(app.error(),"APPLICATION / RESOURCE MAP"));
    assert(app.open(bytes.data(),bytes.size(),expected));app.close();
    assert(!app.resource(0x434f4445,0,code0));
    std::puts("APPLICATION PREFLIGHT PASS; changed input, truncation, null inputs, malformed map/A5/jump table rejected");
}
