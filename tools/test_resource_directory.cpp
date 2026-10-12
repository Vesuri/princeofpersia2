#include <cassert>
#include <cstdio>
#include <vector>
#include <algorithm>
#include <fstream>
#include <iterator>
#include "../src/mac/ResourceDirectory.h"
#include "../src/mac/ResourceMap.h"
static uint32_t lng(const uint8_t* p) { return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3]; }
struct Data {
    std::vector<uint8_t> bytes;uint32_t calls=0,total=0,fail=0;bool shortRead=false,metadataOnly=false;
    static int32_t read(void* context,uint32_t at,uint8_t* out,uint32_t size,uint32_t& actual) {
        auto& s=*(Data*)context;++s.calls;s.total+=size;
        assert(size<=65536 && at<=s.bytes.size() && size<=s.bytes.size()-at);
        if(s.metadataOnly) {
            ResourceMap map;const auto* p=s.bytes.data();assert(map.open(p,s.bytes.size(),p+lng(p+4),lng(p+12)));
            bool allowed=(at==0 && size==16) || (at==lng(p+4) && size==lng(p+12));
            for(uint16_t i=0;i<map.count();++i) { ResourceMap::Entry e;assert(map.entry(i,e));if(at==e.lengthOffset && size==4)allowed=true; }
            assert(allowed);
        }
        if(s.calls==s.fail) { actual=0;return -36; }
        actual=s.shortRead ? size-1 : size;std::copy_n(s.bytes.data()+at,actual,out);return 0;
    }
    ResourceForks::Source source() { return {this,(uint32_t)bytes.size(),read}; }
};
struct Sink {
    std::vector<uint8_t> target={1,2,3},stage;bool fail=false;
    static int32_t begin(void* c,uint32_t n) { ((Sink*)c)->stage.assign(n,0);return 0; }
    static int32_t write(void* c,uint32_t at,const uint8_t* b,uint32_t n,uint32_t& actual) {
        auto& s=*(Sink*)c;assert(at+n<=s.stage.size());if(s.fail) { actual=0;return -36; }
        std::copy_n(b,n,s.stage.begin()+at);actual=n;return 0;
    }
    static int32_t finish(void* c,bool commit) { auto& s=*(Sink*)c;if(commit)s.target.swap(s.stage);s.stage.clear();return 0; }
    ResourceWriter::Sink sink() { return {this,begin,write,finish}; }
};
static void save(const char* path,const std::vector<uint8_t>& b) { std::ofstream f(path,std::ios::binary);f.write((const char*)b.data(),b.size());assert(f.good()); }
int main(int argc,char** argv) {
    Data body;body.bytes={10,20,30,40};uint8_t name[]={'O','l','d'};
    ResourceWriter::Entry e={0x54455354,128,0x28,name,3,body.source(),0,4};
    auto second=e;second.id=-3;second.name=0;second.nameLength=0;
    ResourceWriter::Entry initial[]={e,second};Sink sourceSink;assert(!ResourceWriter::serialize(initial,2,sourceSink.sink()));
    Data source;source.bytes=sourceSink.target;source.metadataOnly=true;
    ResourceDirectory directory;assert(!directory.open(10,source.source(),true));assert(source.calls==4 && !directory.dirty(10));
    assert(!directory.open(20,source.source(),false));assert(source.calls==8);
    ResourceDirectory::View a={},b={},v={};assert(directory.find(10,e.type,e.id,a) && directory.find(20,e.type,e.id,b) && a.identity!=b.identity);
    assert(directory.at(10,0,v) && v.identity==a.identity);assert(directory.at(10,1,v) && v.entry.id==-3);uint32_t removed=v.identity;
    int16_t ref=0;assert(directory.newest(ref) && ref==20);assert(directory.older(20,ref) && ref==10 && !directory.older(10,ref));
    auto calls=source.calls;assert(directory.open(10,source.source(),true)==-48 && source.calls==calls);
    uint32_t id=0xabcdef;assert(directory.add(20,e,id)==-54 && id==0xabcdef);assert(directory.replace(b.identity,e)==-54);
    assert(!directory.remove(b.identity) && !directory.get(b.identity,v) && directory.dirty(20));
    Sink readonly;assert(directory.serialize(20,readonly.sink())==-54 && readonly.target==std::vector<uint8_t>({1,2,3}));
    assert(directory.find(10,e.type,e.id,v) && v.identity==a.identity && source.calls==calls);
    auto invalid=e;invalid.size=0xffffffff;assert(directory.replace(a.identity,invalid)==-50 && !directory.dirty(10));
    assert(directory.get(a.identity,v) && v.entry.size==4 && v.entry.name[0]=='O');
    Data changed;changed.bytes.resize(70001);for(uint32_t i=0;i<changed.bytes.size();++i)changed.bytes[i]=i*19;
    uint8_t renamed[]={'N','e','w'};auto replacement=e;replacement.name=renamed;replacement.attrs=0x10;replacement.source=changed.source();replacement.size=changed.bytes.size();
    assert(!directory.replace(a.identity,replacement) && directory.dirty(10));renamed[0]='X';
    assert(directory.get(a.identity,v) && v.entry.name[0]=='N' && v.identity==a.identity);
    assert(!directory.remove(removed) && !directory.get(removed,v));
    auto added=second;added.id=7;added.attrs=0;added.offset=1;added.size=2;
    assert(!directory.add(10,added,id) && id!=removed && id!=a.identity);
    assert(directory.at(10,1,v) && v.identity==id && source.calls==calls);
    Sink failed;failed.fail=true;source.metadataOnly=false;
    assert(directory.serialize(10,failed.sink())==-36 && failed.target==std::vector<uint8_t>({1,2,3}) && directory.dirty(10));
    Sink result;assert(!directory.serialize(10,result.sink()) && directory.dirty(10));if(argc>1)save(argv[1],result.target);
    assert(directory.rebase(10,source.source())==-50 && directory.dirty(10));
    Data published;published.bytes=result.target;published.metadataOnly=true;assert(!directory.rebase(10,published.source()) && !directory.dirty(10));
    assert(published.calls==4 && directory.get(a.identity,v) && v.entry.name[0]=='N' && directory.get(id,v));
    published.metadataOnly=false;changed.bytes.clear();uint8_t data[4];assert(!directory.read(a.identity,0,data,4));
    for(uint16_t i=0;i<4;++i)assert(data[i]==i*19);
    assert(!directory.read(id,0,data,2) && data[0]==20 && data[1]==30);
    assert(directory.read(id,2,data,1)==-50 && directory.read(removed,0,data,1)==-192);
    // Failed opens and rebases leave existing maps/identities intact.
    Data bad;bad.bytes=published.bytes;bad.shortRead=true;
    assert(directory.open(30,bad.source(),true)==-39 && !directory.active(30));
    assert(directory.rebase(10,bad.source())==-39 && directory.get(a.identity,v));
    bad.shortRead=false;bad.bytes[0]=0xff;assert(directory.open(30,bad.source(),true)==-50);
    assert(!directory.close(20) && directory.get(a.identity,v) && !directory.get(b.identity,v));
    assert(!directory.open(20,source.source(),false) && directory.find(20,e.type,e.id,v) && v.identity!=b.identity);
    assert(directory.newest(ref) && ref==20);
    directory.clear();assert(!directory.get(a.identity,v) && !directory.active(10));
    // Same-file duplicates retain independent identity/body even after slot reuse.
    assert(!directory.create(10));uint32_t first,older,newer;
    auto duplicate=e;duplicate.size=1;
    assert(!directory.add(10,duplicate,first));duplicate.offset=1;
    assert(!directory.add(10,duplicate,older));
    assert(directory.find(10,e.type,e.id,v) && v.identity==first);
    assert(!directory.remove(first));duplicate.offset=2;
    assert(!directory.add(10,duplicate,newer)); // Reuses first's physical slot.
    assert(first!=older && older!=newer && first!=newer);
    assert(directory.find(10,e.type,e.id,v) && v.identity==older);
    assert(directory.at(10,0,v) && v.identity==older && directory.at(10,1,v) && v.identity==newer);
    duplicate.offset=3;assert(!directory.replace(older,duplicate));
    Sink duplicates;assert(!directory.serialize(10,duplicates.sink()));
    if(argc>1)save((std::string(argv[1])+".duplicates").c_str(),duplicates.target);
    Data duplicateDisk;duplicateDisk.bytes=duplicates.target;duplicateDisk.metadataOnly=true;
    assert(!directory.rebase(10,duplicateDisk.source()) && duplicateDisk.calls==4);
    assert(directory.find(10,e.type,e.id,v) && v.identity==older && !directory.get(first,v));
    duplicateDisk.metadataOnly=false;
    assert(!directory.read(older,0,data,1) && data[0]==40);
    assert(!directory.read(newer,0,data,1) && data[0]==30);
    assert(!directory.remove(older) && directory.find(10,e.type,e.id,v) && v.identity==newer);
    directory.clear();duplicateDisk.metadataOnly=true;duplicateDisk.calls=0;
    assert(!directory.open(10,duplicateDisk.source(),true) && duplicateDisk.calls==4);
    assert(directory.find(10,e.type,e.id,v) && v.identity!=older && v.identity!=newer);
    duplicateDisk.metadataOnly=false;assert(!directory.read(v.identity,0,data,1) && data[0]==40);
    directory.clear();
    // Exhaust each independent capacity without changing existing maps.
    for(uint16_t i=0;i<16;++i)assert(!directory.create(i));
    assert(directory.create(16)==-108 && directory.newest(ref) && ref==15);directory.clear();
    std::vector<ResourceWriter::Entry> full(768,second);for(uint16_t i=0;i<768;++i) { full[i].id=i;full[i].size=0; }
    Sink fullSink;assert(!ResourceWriter::serialize(full.data(),full.size(),fullSink.sink()));
    Data fullSource;fullSource.bytes=fullSink.target;assert(!directory.open(1,fullSource.source(),true));
    assert(directory.add(1,e,id)==-108 && directory.count(1)==768);
    assert(directory.open(2,source.source(),true)==-108 && !directory.active(2) && directory.count(1)==768);
    if(argc>3) {
        directory.clear();Data original;std::ifstream file(argv[2],std::ios::binary);
        original.bytes.assign(std::istreambuf_iterator<char>(file),{});original.metadataOnly=true;
        assert(!directory.open(100,original.source(),true) && original.calls==214);
        std::vector<uint32_t> identities;
        for(uint16_t i=0;i<directory.count(100);++i) { assert(directory.at(100,i,v));identities.push_back(v.identity); }
        original.metadataOnly=false;Sink copy;assert(!directory.serialize(100,copy.sink()));save(argv[3],copy.target);
        Data rebased;rebased.bytes=copy.target;rebased.metadataOnly=true;assert(!directory.rebase(100,rebased.source()) && rebased.calls==214);
        for(uint16_t i=0;i<identities.size();++i)assert(directory.at(100,i,v) && v.identity==identities[i]);
        std::puts("PASS resource directory original: 212 entries, metadata-only open/rebase, stable identities and streamed serialization");
    }
    std::puts("PASS resource directory: metadata-only opens, independent identities, ordered mutations, duplicate identity/order, readonly/overflow atomicity, staged serialization and identity-preserving rebase");
}
