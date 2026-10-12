#include <initializer_list>

#include <cassert>
#include <cstdio>
#include <cstring>
#include "../src/mac/Identity.h"
#include "../src/mac/SHA256.h"
int main() {
    SHA256 sha;unsigned char hash[32];sha_init(&sha);sha_update(&sha,(const unsigned char*)"abc",3);sha_final(&sha,hash);
    const char* expected="ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";
    char hex[65];for(int i=0;i<32;++i)std::snprintf(hex+2*i,3,"%02x",hash[i]);assert(!std::strcmp(hex,expected));
    unsigned char env[18];std::memset(env,0x55,sizeof env);
    assert(MacIdentity::sysEnvirons(1,env+1) && env[0]==0x55 && env[17]==0x55);
    assert(!MacIdentity::sysEnvirons(2,env+1) && !MacIdentity::sysEnvirons(1,nullptr));
    for(auto selector:{0x766d2020UL,0x70726f63UL,0x73797376UL,0x6d616368UL,0x6d6d7520UL,0x61646472UL,0x612f7578UL}) {
        uint32_t response;int16_t error;assert(MacIdentity::gestalt(selector,response,error));
        std::printf("IDENTITY %08x %08x %04x\n",unsigned(selector),unsigned(response),uint16_t(error));
    }
    uint32_t response;int16_t error;assert(!MacIdentity::gestalt(0x12345678,response,error));
    std::printf("SYSENV ");for(int i=1;i<=16;++i)std::printf("%02x",env[i]);std::puts("");
    std::puts("PASS SHA256 standard vector; identity guards and measured answers");
}
