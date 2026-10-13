#include <cassert>
#include <cstdio>
#include <vector>
#include <algorithm>
#include "../src/mac/ResourceDirectory.h"
// Independent insertion-order oracle: mutations must never retain a stale key,
// return a younger duplicate, or confuse a reopened fork with its old contents.
struct Expected {int16_t fork;uint32_t identity;ResourceWriter::Entry entry;};
int main() {
    ResourceDirectory directory;
    assert(!directory.create(10) && !directory.create(20));
    std::vector<Expected> expected;
    uint32_t random=0x12345678;
    auto next=[&]() {random=random*1664525+1013904223;return random;};
    auto verify=[&]() {
        for(int16_t fork: {10,20})for(uint32_t type=1;type<=4;++type)for(int16_t id=-8;id<=8;++id) {
            const Expected* first=nullptr;
            for(const auto& e:expected)if(e.fork==fork && e.entry.type==type && e.entry.id==id && (!first || e.identity<first->identity))first=&e;
            ResourceDirectory::View found={};
            assert(directory.find(fork,type,id,found)==bool(first));
            if(first)assert(found.identity==first->identity && found.ref==fork && found.entry.attrs==first->entry.attrs);
        }
    };
    for(unsigned step=0;step<400;++step) {
        const int16_t fork=next()&1?10:20;
        ResourceWriter::Entry entry={};entry.source.read=[](void*,uint32_t,uint8_t*,uint32_t,uint32_t& actual)->int32_t {actual=0;return 0;};entry.type=1+(next()%4);entry.id=int16_t(next()%17)-8;entry.attrs=uint8_t(next()&0x70);
        unsigned action=next()%4;
        if(expected.empty() || action==0) {
            uint32_t identity=0;assert(!directory.add(fork,entry,identity));expected.push_back({fork,identity,entry});
        } else {
            unsigned index=next()%expected.size();auto& e=expected[index];
            if(action==1) {assert(!directory.replace(e.identity,entry));e.entry=entry;}
            else if(action==2) {assert(!directory.remove(e.identity));expected.erase(expected.begin()+index);}
            else {uint32_t identity=0;assert(!directory.add(e.fork,e.entry,identity));expected.push_back({e.fork,identity,e.entry});}
        }
        verify();
        if(step%61==60) {
            assert(!directory.close(fork));
            expected.erase(std::remove_if(expected.begin(),expected.end(),[&](const Expected& e){return e.fork==fork;}),expected.end());
            assert(!directory.create(fork));verify();
        }
    }
    directory.clear();expected.clear();assert(!directory.create(10) && !directory.create(20));verify();
    std::puts("PASS resource lookup: 400 mutations against insertion-order oracle, duplicate keys, signed IDs, replacement, slot reuse and reopened forks");
}
