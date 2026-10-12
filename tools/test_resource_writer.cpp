#include <cassert>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <vector>
#include <algorithm>
#include "../src/mac/ResourceWriter.h"
#include "../src/mac/ResourceMap.h"
struct Input {
    std::vector<uint8_t> bytes;uint32_t calls=0,total=0,fail=0;bool shortRead=false;
    static int32_t read(void* context,uint32_t at,uint8_t* out,uint32_t size,uint32_t& actual) {
        auto& s=*(Input*)context;++s.calls;s.total+=size;
        assert(size<=65536 && at<=s.bytes.size() && size<=s.bytes.size()-at);
        if(s.calls==s.fail) { actual=0;return -36; }
        actual=shortReadSize(s.shortRead,size);std::copy_n(s.bytes.data()+at,actual,out);return 0;
    }
    static uint32_t shortReadSize(bool shortRead,uint32_t size) { return shortRead ? size-1 : size; }
    ResourceForks::Source source() { return {this,(uint32_t)bytes.size(),read}; }
};
struct Output {
    std::vector<uint8_t> target={0x11,0x22,0x33},stage;
    uint32_t writes=0,begins=0,commits=0,aborts=0,fail=0;bool shortWrite=false,failBegin=false,failCommit=false,failAbort=false;
    static int32_t begin(void* c,uint32_t size) { auto& s=*(Output*)c;++s.begins;s.stage.assign(size,0xcc);return s.failBegin ? -36 : 0; }
    static int32_t write(void* c,uint32_t at,const uint8_t* bytes,uint32_t size,uint32_t& actual) {
        auto& s=*(Output*)c;++s.writes;assert(size<=65536 && at<=s.stage.size() && size<=s.stage.size()-at);
        assert(s.target==std::vector<uint8_t>({0x11,0x22,0x33}));
        if(s.writes==s.fail) { actual=0;return -36; }
        actual=s.shortWrite ? size-1 : size;std::copy_n(bytes,actual,s.stage.begin()+at);return 0;
    }
    static int32_t finish(void* c,bool publish) {
        auto& s=*(Output*)c;
        if(publish) { ++s.commits;if(s.failCommit)return -36;s.target.swap(s.stage); }
        else { ++s.aborts;if(s.failAbort)return -32760; }
        s.stage.clear();return 0;
    }
    ResourceWriter::Sink sink() { return {this,begin,write,finish}; }
};
static uint32_t lng(const uint8_t* p) { return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3]; }
static void save(const char* path,const std::vector<uint8_t>& bytes) { std::ofstream f(path,std::ios::binary);f.write((const char*)bytes.data(),bytes.size());assert(f.good()); }
static void checkMap(const std::vector<uint8_t>& data,uint16_t count) {
    ResourceMap map;assert(map.open(data.data(),data.size(),data.data()+lng(data.data()+4),lng(data.data()+12)));assert(map.count()==count);
}
int main(int argc,char** argv) {
    Input input;input.bytes.resize(100003);for(uint32_t i=0;i<input.bytes.size();++i)input.bytes[i]=(i*37+(i>>8))&255;
    const uint8_t name[]={0x41,0x8e,0x00};const uint8_t emptyName[]={0};
    ResourceWriter::Entry entries[]={
        {0x54455354,128,0x28,name,3,input.source(),0,100003},
        {0x4f544852,-3,0,0,0,input.source(),0,0},
        {0x54455354,-1,0x10,emptyName,0,input.source(),17,3}
    };
    Output output;assert(!ResourceWriter::serialize(entries,3,output.sink()));checkMap(output.target,3);
    assert(input.calls==3 && input.total==100006 && output.commits==1 && !output.aborts);
    ResourceMap map;assert(map.open(output.target.data(),output.target.size(),output.target.data()+lng(output.target.data()+4),lng(output.target.data()+12)));
    ResourceMap::Entry item;assert(map.entry(0,item) && item.id==128 && item.nameLength==3);
    assert(map.entry(1,item) && item.id==-1 && item.name && !item.nameLength);
    assert(map.entry(2,item) && item.id==-3 && !item.name);
    if(argc>1)save(argv[1],output.target);
    // Every write boundary must leave the prior target intact when it fails.
    for(uint32_t fail=1;fail<=output.writes;++fail) {
        Output bad;bad.fail=fail;assert(ResourceWriter::serialize(entries,3,bad.sink())==-36);
        assert(bad.target==std::vector<uint8_t>({0x11,0x22,0x33}) && bad.aborts==1 && !bad.commits && bad.stage.empty());
    }
    for(uint16_t mode=0;mode<5;++mode) {
        Output bad;bad.shortWrite=mode==0;bad.failBegin=mode==1;bad.failCommit=mode==2;
        input.shortRead=mode==3;input.fail=mode==4 ? input.calls+2 : 0;
        assert(ResourceWriter::serialize(entries,3,bad.sink())==(mode==0 || mode==3 ? -39 : -36));
        assert(bad.target==std::vector<uint8_t>({0x11,0x22,0x33}) && bad.aborts==1 && bad.stage.empty());
        input.shortRead=false;input.fail=0;
    }
    { Output bad;bad.failCommit=true;bad.failAbort=true;
      assert(ResourceWriter::serialize(entries,3,bad.sink())==-32760);
      assert(bad.aborts==1 && bad.commits==1 && !bad.stage.empty());
      assert(bad.target==std::vector<uint8_t>({0x11,0x22,0x33})); }
    for(uint16_t mode=0;mode<4;++mode) {
        auto badEntry=entries[0];Output bad;
        if(mode==0)badEntry.offset=100004;
        if(mode==1)badEntry.size=0xffffffff;
        if(mode==2)badEntry.name=0;
        if(mode==3)badEntry.source.read=0;
        assert(ResourceWriter::serialize(&badEntry,1,bad.sink())==-50 && !bad.begins);
    }
    { auto duplicate=entries[0];duplicate.offset=17;duplicate.size=3;duplicate.attrs=0x10;duplicate.name=0;duplicate.nameLength=0;
      ResourceWriter::Entry duplicates[]={entries[0],entries[1],duplicate};Output saved;
      auto calls=input.calls;uint32_t size;
      assert(!ResourceWriter::measure(duplicates,3,size) && input.calls==calls);
      assert(!ResourceWriter::serialize(duplicates,3,saved.sink()) && saved.target.size()==size);checkMap(saved.target,3);
      if(argc>1)save((std::string(argv[1])+".duplicates").c_str(),saved.target); }
    { ResourceWriter::Entry tooFar[]={entries[0],entries[1]};Output bad;
      tooFar[0].source.size=0xffffffff;tooFar[0].size=0xffffff;
      assert(ResourceWriter::serialize(tooFar,2,bad.sink())==-50 && !bad.begins); }
    { Output bad;assert(ResourceWriter::serialize(entries,769,bad.sink())==-50 && !bad.begins); }
    { Output empty;assert(!ResourceWriter::serialize(0,0,empty.sink()));checkMap(empty.target,0);if(argc>2)save(argv[2],empty.target); }
    // Name-list offsets are 16-bit even though map lengths are 32-bit.
    { std::vector<ResourceWriter::Entry> many(768,entries[0]);uint8_t longName[255]={};
      for(uint16_t i=0;i<many.size();++i) { many[i].id=i;many[i].name=longName;many[i].nameLength=255;many[i].size=0; }
      Output bad;assert(ResourceWriter::serialize(many.data(),many.size(),bad.sink())==-50 && !bad.begins); }
    if(argc>4) {
        Input original;std::ifstream f(argv[3],std::ios::binary);original.bytes.assign(std::istreambuf_iterator<char>(f),{});assert(original.bytes.size()>16);
        ResourceMap source;const uint8_t* b=original.bytes.data();assert(source.open(b,original.bytes.size(),b+lng(b+4),lng(b+12)));
        std::vector<ResourceWriter::Entry> recipe;
        uint32_t total=0;
        for(uint16_t i=0;i<source.count();++i) {
            ResourceMap::Entry e;assert(source.entry(i,e));uint32_t size=lng(b+e.lengthOffset),offset;
            assert(source.payload(i,size,offset));recipe.push_back({e.type,e.id,e.attrs,e.name,e.nameLength,original.source(),offset,size});total+=size;
        }
        Output result;assert(!ResourceWriter::serialize(recipe.data(),recipe.size(),result.sink()));
        assert(original.total==total);checkMap(result.target,source.count());save(argv[4],result.target);
        std::printf("PASS resource writer original: entries=%u payload-bytes=%u bounded-reads=%u\n",source.count(),total,original.calls);
    }
    std::puts("PASS resource writer: order/names/empty, bounded streaming, duplicate order, overflow rejection, staged write/read/commit failure atomicity");
}
