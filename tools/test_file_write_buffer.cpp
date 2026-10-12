#include <cassert>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <map>
#include "../src/mac/FileWriteBuffer.h"
static int allocations=0,failAfter=-1;
static std::map<uint8_t*,uint32_t> sizes;
static uint8_t* allocate(uint32_t size) {
    assert(size && size<=2*1024*1024);
    if(failAfter==0)return nullptr;
    if(failAfter>0)--failAfter;
    ++allocations;auto* p=new uint8_t[size];sizes[p]=size;return p;
}
static void release(uint8_t* p,uint32_t size) { assert(allocations>0 && sizes.at(p)==size);sizes.erase(p);--allocations;delete[] p; }
struct Disk {
    std::vector<uint8_t> bytes;
    unsigned reads=0,writes=0,resizes=0;
    bool shortRead=false,failRead=false,shortWrite=false,failResize=false,failReplace=false;
};
static int32_t readDisk(void* c,uint32_t off,uint8_t* dest,uint32_t n,uint32_t& actual) {
    auto& d=*(Disk*)c;assert(n<=65536 && off+n<=d.bytes.size());++d.reads;
    actual=d.shortRead ? n/2 : n;memcpy(dest,d.bytes.data()+off,actual);return d.failRead ? -36 : 0;
}
static int32_t writeDisk(void* c,uint32_t off,const uint8_t* src,uint32_t n,uint32_t& actual) {
    auto& d=*(Disk*)c;assert(n<=65536);++d.writes;actual=d.shortWrite ? n/2 : n;
    if(off+actual>d.bytes.size())d.bytes.resize(off+actual,0xcc);
    memcpy(d.bytes.data()+off,src,actual);return 0;
}
static int32_t resizeDisk(void* c,uint32_t size) {
    auto& d=*(Disk*)c;++d.resizes;if(d.failResize)return -36;
    d.bytes.resize(size,0xdd);return 0;
}
static int32_t replaceDisk(void* c,const uint8_t* bytes,uint32_t size) {
    auto& d=*(Disk*)c;
    if(d.failReplace)return -36;
    ++d.writes;d.bytes.assign(bytes,bytes+size);return 0;
}
static std::vector<uint8_t> pattern(uint32_t n) {
    std::vector<uint8_t> r(n);for(uint32_t i=0;i<n;++i)r[i]=(i*37+(i>>8))&255;return r;
}
static void check(FileWriteBuffer& b,const std::vector<uint8_t>& expected) {
    assert(b.size()==expected.size());std::vector<uint8_t> got(expected.size()+7,0xcc);uint32_t actual=999;
    assert(b.read(0,got.data(),got.size(),actual)==-39 && actual==expected.size());
    assert(std::equal(expected.begin(),expected.end(),got.begin()));
    for(unsigned i=actual;i<got.size();++i)assert(got[i]==0xcc);
}
int main() {
    FileWriteBuffer b;Disk d;d.bytes=pattern(200003);auto expected=d.bytes;
    assert(b.bind(d.bytes.size(),readDisk,&d,allocate,release)==0 && !b.dirty() && d.reads==0);
    uint32_t actual;auto data=pattern(131089);
    assert(b.write(65530,data.data(),data.size(),actual)==0 && actual==data.size() && d.writes==0 && d.reads==2);
    assert(b.bind(0,readDisk,&d,allocate,release)==-50 && b.dirty() && b.size()==200003);
    std::copy(data.begin(),data.end(),expected.begin()+65530);check(b,expected);
    assert(b.flush(writeDisk,resizeDisk,&d)==0 && d.bytes==expected && !b.dirty() && allocations==0);
    unsigned written=d.writes,resized=d.resizes;
    assert(b.flush(writeDisk,resizeDisk,&d)==0 && d.writes==written && d.resizes==resized);
    assert(b.resize(17)==0);expected.resize(17);
    assert(b.resize(200003)==0);expected.resize(200003,0);check(b,expected);
    d.shortWrite=true;assert(b.flush(writeDisk,resizeDisk,&d)==-36 && b.dirty());
    d.shortWrite=false;check(b,expected);
    d.failResize=true;assert(b.flush(writeDisk,resizeDisk,&d)==-36 && b.dirty());
    d.failResize=false;assert(b.flush(writeDisk,resizeDisk,&d)==0 && d.bytes==expected && !b.dirty());
    // A failed/short fill never installs a page; read counts retain actual bytes.
    d.shortRead=true;d.failRead=true;
    unsigned live=allocations;
    assert(b.write(3,data.data(),4,actual)==-36 && actual==0 && allocations==(int)live && !b.dirty());
    uint8_t out[20];assert(b.read(0,out,20,actual)==-36 && actual==10);
    d.failRead=false;assert(b.write(3,data.data(),4,actual)==-36 && actual==0 && !b.dirty());
    d.shortRead=false;
    // Allocation failure after a full-page prefix is a partial write, not success.
    failAfter=1;assert(b.write(0,data.data(),data.size(),actual)==-108 && actual==65536 && b.dirty());
    failAfter=-1;std::copy(data.begin(),data.begin()+actual,expected.begin());check(b,expected);
    b.clear();assert(allocations==0);
    assert(b.bind(0,readDisk,&d,allocate,release)==0);
    assert(b.write(0,nullptr,0,actual)==0 && !b.dirty());
    assert(b.write(0x7fffffff,data.data(),1,actual)==-50 && !b.dirty());
    assert(b.resize(0x80000000)==-50 && !b.dirty());
    assert(b.read(0,nullptr,0,actual)==0 && actual==0);
    for(unsigned i=0;i<FileWriteBuffer::maxPages;++i)assert(b.write(i*65536,data.data(),65536,actual)==0);
    assert(b.write(FileWriteBuffer::maxPages*65536,data.data(),1,actual)==b.unsupported && actual==0);
    b.clear();assert(allocations==0);
    // Deterministic operation sequences versus a byte-vector oracle, including holes.
    d.bytes=pattern(50000);expected=d.bytes;assert(b.bind(d.bytes.size(),readDisk,&d,allocate,release)==0);
    uint32_t rng=42;
    for(unsigned i=0;i<300;++i) {
        rng=rng*1664525+1013904223;uint32_t off=rng%250000;
        if(i%5==0) { assert(b.resize(off)==0);expected.resize(off,0); }
        else {
            uint32_t count=(rng>>16)%10000;
            assert(b.write(off,data.data(),count,actual)==0 && actual==count);
            if(count) { if(off+count>expected.size())expected.resize(off+count,0);std::copy(data.begin(),data.begin()+count,expected.begin()+off); }
        }
        check(b,expected);
        if(i%11==0) { assert((i%22 ? b.flush(writeDisk,resizeDisk,&d) : b.flushWhole(replaceDisk,&d))==0 && d.bytes==expected); }
    }
    assert(b.flush(writeDisk,resizeDisk,&d)==0 && d.bytes==expected);
    assert(b.resize(0)==0 && b.dirty());written=d.writes;
    assert(b.flush(writeDisk,resizeDisk,&d)==0 && d.bytes.empty() && d.writes==written && !b.dirty());
    b.clear();assert(allocations==0);
    // Whole-file publication keeps both backing and dirty overlay on failures.
    d.bytes=pattern(200003);expected=d.bytes;
    assert(b.bind(d.bytes.size(),readDisk,&d,allocate,release)==0);
    assert(b.write(7,data.data(),32,actual)==0);std::copy(data.begin(),data.begin()+32,expected.begin()+7);
    auto original=d.bytes;failAfter=0;
    assert(b.flushWhole(replaceDisk,&d)==-108 && b.dirty() && d.bytes==original);
    failAfter=-1;d.failReplace=true;
    assert(b.flushWhole(replaceDisk,&d)==-36 && b.dirty() && d.bytes==original);
    d.failReplace=false;d.shortRead=true;
    assert(b.flushWhole(replaceDisk,&d)==-36 && b.dirty() && d.bytes==original);
    d.shortRead=false;check(b,expected);
    assert(b.flushWhole(replaceDisk,&d)==0 && !b.dirty() && d.bytes==expected && allocations==0);
    assert(b.resize(17)==0 && b.flushWhole(replaceDisk,&d)==0 && d.bytes.size()==17);
    assert(b.resize(0)==0 && b.flushWhole(replaceDisk,&d)==0 && d.bytes.empty());
    b.clear();assert(allocations==0 && sizes.empty());
    puts("PASS file-write-buffer: lazy bounded pages, byte oracle, truncate/regrow, partial errors, retryable flush, capacity/cleanup");
}
