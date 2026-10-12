#include <cassert>
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <vector>
#include "../src/mac/ResourceDirectory.h"
#include "../src/mac/ResourceMap.h"
static uint32_t lng(const uint8_t* p) { return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3]; }
struct Data {
    std::vector<uint8_t> bytes;uint32_t calls=0,total=0;bool metadata=false,fail=false;
    static int32_t read(void* c,uint32_t at,uint8_t* out,uint32_t n,uint32_t& actual) {
        auto& d=*(Data*)c;++d.calls;d.total+=n;actual=0;
        assert(n<=65536 && at<=d.bytes.size() && n<=d.bytes.size()-at);
        if(d.metadata) {
            ResourceMap map;auto* b=d.bytes.data();assert(map.open(b,d.bytes.size(),b+lng(b+4),lng(b+12)));
            bool valid=(at==0 && n==16)||(at==lng(b+4) && n==lng(b+12));
            for(uint16_t i=0;i<map.count();++i) { ResourceMap::Entry e;assert(map.entry(i,e));if(at==e.lengthOffset && n==4)valid=true; }
            assert(valid);
        }
        if(d.fail)return -36;
        std::copy_n(d.bytes.data()+at,n,out);actual=n;return 0;
    }
    ResourceForks::Source source() { return {this,(uint32_t)bytes.size(),read}; }
};
struct Sink {
    std::vector<uint8_t> target={1,2,3},stage;uint32_t begins=0,writes=0,failAt=0;bool failCommit=false;
    static int32_t begin(void* c,uint32_t n) { auto& s=*(Sink*)c;++s.begins;s.stage.assign(n,0);return 0; }
    static int32_t write(void* c,uint32_t at,const uint8_t* b,uint32_t n,uint32_t& actual) {
        auto& s=*(Sink*)c;actual=0;if(++s.writes==s.failAt)return -36;
        assert(at<=s.stage.size() && n<=s.stage.size()-at);std::copy_n(b,n,s.stage.data()+at);actual=n;return 0;
    }
    static int32_t finish(void* c,bool commit) { auto& s=*(Sink*)c;if(commit && s.failCommit)return -36;if(commit)s.target.swap(s.stage);s.stage.clear();return 0; }
    ResourceWriter::Sink sink() { return {this,begin,write,finish}; }
};
struct Selection {
    uint32_t identity;Data* body;int32_t error=0;uint32_t calls=0;bool invalid=false;uint32_t omit=0;
    static int32_t select(void* c,uint32_t id,ResourceForks::Source& source,uint32_t& offset,uint32_t& size) {
        auto& s=*(Selection*)c;++s.calls;if(s.error)return s.error;if(id==s.omit)return ResourceDirectory::omitEntry;
        if(id==s.identity) { source=s.body->source();offset=0;size=s.invalid ? source.size+1 : source.size; }return 0;
    }
};
static void save(const char* path,const std::vector<uint8_t>& bytes) { std::ofstream f(path,std::ios::binary);f.write((const char*)bytes.data(),bytes.size());assert(f.good()); }
int main(int argc,char** argv) {
    Data baseline;baseline.bytes={'A','A','A','A','B','B','B','B'};
    uint8_t name[]={'S'};ResourceWriter::Entry recipe[]={{0x49534f4c,128,0,name,1,baseline.source(),0,4},{0x49534f4c,129,0,name,1,baseline.source(),4,4}};
    Sink initial;assert(!ResourceWriter::serialize(recipe,2,initial.sink()));
    Data disk;disk.bytes=initial.target;disk.metadata=true;ResourceDirectory dir;
    assert(!dir.open(7,disk.source(),true) && disk.calls==4);
    ResourceDirectory::View a,b,v;assert(dir.find(7,recipe[0].type,128,a) && dir.find(7,recipe[0].type,129,b));
    Data changedA,changedB;changedA.bytes.assign(70001,'C');changedB.bytes.assign(3,'D');
    Selection selectA={a.identity,&changedA},selectB={b.identity,&changedB};
    // Selection failures and invalid ranges occur before staging starts.
    for(int mode=0;mode<2;++mode) {
        Sink failed;selectA.error=mode==0 ? -36 : 0;selectA.invalid=mode==1;
        assert(dir.serialize(7,failed.sink(),Selection::select,&selectA)==(mode==0 ? -36 : -50));
        assert(!failed.begins && !changedA.calls && !changedB.calls && disk.calls==4);
    }
    selectA.error=0;selectA.invalid=false;disk.metadata=false;
    Sink onlyA;assert(!dir.serialize(7,onlyA.sink(),Selection::select,&selectA));
    assert(changedA.total==70001 && changedB.calls==0 && !dir.dirty(7));
    assert(dir.get(a.identity,v) && v.entry.size==4); // Serialization alone never publishes metadata.
    for(uint32_t boundary=1;boundary<=onlyA.writes+2;++boundary) {
        Sink failed;failed.failAt=boundary<=onlyA.writes ? boundary : 0;
        failed.failCommit=boundary==onlyA.writes+1;changedA.fail=boundary==onlyA.writes+2;
        assert(dir.serialize(7,failed.sink(),Selection::select,&selectA)==-36);
        assert(failed.target==std::vector<uint8_t>({1,2,3}) && failed.stage.empty());
        assert(dir.get(a.identity,v) && v.entry.size==4 && !dir.dirty(7));
    }
    changedA.fail=false;
    Data publishedA;publishedA.bytes=onlyA.target;publishedA.metadata=true;
    assert(dir.rebase(7,publishedA.source())==-50); // Changed size needs the same selection.
    selectA.error=-36;assert(dir.rebase(7,publishedA.source(),Selection::select,&selectA)==-36);
    assert(dir.get(a.identity,v) && v.entry.size==4);selectA.error=0;
    auto beforeA=changedA.calls;publishedA.calls=0;
    assert(!dir.rebase(7,publishedA.source(),Selection::select,&selectA));
    assert(publishedA.calls==4 && changedA.calls==beforeA && changedB.calls==0);
    assert(dir.get(a.identity,v) && v.entry.size==70001 && dir.get(b.identity,v) && v.entry.size==4);
    publishedA.metadata=false;uint8_t bytes[4];assert(!dir.read(a.identity,0,bytes,4) && bytes[0]=='C');
    assert(!dir.read(b.identity,0,bytes,4) && bytes[0]=='B');
    changedA.bytes.assign(70001,'X'); // The written body no longer borrows the live handle.
    assert(!dir.read(a.identity,0,bytes,4) && bytes[0]=='C');
    Sink onlyB;assert(!dir.serialize(7,onlyB.sink(),Selection::select,&selectB));
    Data publishedB;publishedB.bytes=onlyB.target;publishedB.metadata=true;
    assert(!dir.rebase(7,publishedB.source(),Selection::select,&selectB) && publishedB.calls==4);
    publishedB.metadata=false;assert(!dir.read(a.identity,0,bytes,4) && bytes[0]=='C');
    assert(!dir.read(b.identity,0,bytes,3) && bytes[0]=='D');
    assert(dir.at(7,0,v) && v.identity==a.identity && dir.at(7,1,v) && v.identity==b.identity);
    // A new peer has no saved payload. Keep it live without reading/writing it.
    ResourceDirectory pending;assert(!pending.create(9));
    Data freshA,freshB;freshA.bytes.assign(4,'A');freshB.bytes.assign(4,'B');
    auto ea=recipe[0],eb=recipe[1];ea.source=freshA.source();eb.source=freshB.source();eb.offset=0;
    uint32_t aid=0,bid=0;assert(!pending.add(9,ea,aid) && !pending.add(9,eb,bid));
    Selection skipB={aid,&freshA};skipB.omit=bid;
    Sink writtenA;assert(!pending.serialize(9,writtenA.sink(),Selection::select,&skipB));
    assert(!freshB.calls && pending.count(9)==2 && pending.dirty(9));
    for(uint32_t boundary=1;boundary<=writtenA.writes+1;++boundary) {
        Sink failed;failed.failAt=boundary<=writtenA.writes?boundary:0;failed.failCommit=boundary>writtenA.writes;
        assert(pending.serialize(9,failed.sink(),Selection::select,&skipB)==-36);
        assert(failed.target==std::vector<uint8_t>({1,2,3}) && failed.stage.empty());
        assert(pending.get(bid,v) && v.entry.source.context==&freshB && !freshB.calls);
    }
    Data savedA;savedA.bytes=writtenA.target;savedA.metadata=true;
    assert(pending.rebase(9,savedA.source())==-50);
    assert(pending.get(aid,v) && v.entry.source.context==&freshA);
    auto ac=freshA.calls;savedA.calls=0;
    assert(!pending.rebase(9,savedA.source(),Selection::select,&skipB));
    assert(savedA.calls==3 && freshA.calls==ac && !freshB.calls && pending.dirty(9));
    assert(pending.count(9)==2 && pending.get(bid,v) && v.entry.source.context==&freshB && v.entry.name[0]=='S');
    savedA.metadata=false;freshA.bytes.assign(4,'X');
    assert(!pending.read(aid,0,bytes,4) && bytes[0]=='A');
    assert(!pending.read(bid,0,bytes,4) && bytes[0]=='B');
    Sink both;assert(!pending.serialize(9,both.sink()));Data savedBoth;savedBoth.bytes=both.target;savedBoth.metadata=true;
    assert(!pending.rebase(9,savedBoth.source()) && !pending.dirty(9) && pending.count(9)==2);
    savedBoth.metadata=false;
    // Omit a map-borrowed name as well: rebase must retain valid owned metadata.
    Sink withoutB;assert(!pending.serialize(9,withoutB.sink(),Selection::select,&skipB));
    Data secondA;secondA.bytes=withoutB.target;secondA.metadata=true;
    assert(!pending.rebase(9,secondA.source(),Selection::select,&skipB));
    assert(pending.get(bid,v) && v.entry.name && v.entry.name[0]=='S' && pending.dirty(9));
    assert(!pending.read(bid,0,bytes,4) && bytes[0]=='B');
    assert(!pending.remove(bid));freshA.bytes.assign(4,'C');
    Sink removed;assert(!pending.serialize(9,removed.sink(),Selection::select,&skipB));
    Data savedRemoved;savedRemoved.bytes=removed.target;savedRemoved.metadata=true;
    assert(!pending.rebase(9,savedRemoved.source(),Selection::select,&skipB));
    assert(!pending.dirty(9) && pending.count(9)==1 && !pending.get(bid,v) && pending.get(aid,v));
    if(argc>4) { save(argv[3],both.target);save(argv[4],removed.target); }
    if(argc>2) { save(argv[1],onlyA.target);save(argv[2],onlyB.target); }
    std::puts("PASS selective resource publication: independent bodies/sizes, bounded reads, no preload, failed selection/write/read/commit/rebase atomicity, stable identity and saved sources");
}
