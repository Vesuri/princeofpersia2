#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>
#include "mac/FileTransfers.h"
struct Store { std::vector<uint8_t> bytes;bool fail=false,shortRead=false; };
static int32_t read(void* context,uint32_t,bool,uint32_t at,uint8_t* out,uint32_t count,uint32_t& actual) {
    Store& s=*(Store*)context;if(s.fail)return -36;
    actual=count-(s.shortRead && count ? 1 : 0);
    assert(at+actual<=s.bytes.size());std::memcpy(out,s.bytes.data()+at,actual);return 0;
}
static int32_t write(void* context,uint32_t,bool,uint32_t at,const uint8_t* in,uint32_t count,uint32_t& actual) {
    Store& s=*(Store*)context;if(s.fail)return -36;
    s.bytes.resize(std::max<size_t>(s.bytes.size(),at+count));
    std::memcpy(s.bytes.data()+at,in,count);actual=count;return 0;
}
int main() {
    MacFiles files;files.reset();Store s={{1,2,3,4}};
    int32_t id=files.add(2,"save","save",false,4);assert(id>=2);
    int16_t ref=0;assert(files.openData(id,3,false,ref)==0);
    FileTransfers::Backend backend={&s,read,write};uint8_t out[8]={};
    auto r=FileTransfers::read(files,backend,ref,1,2,out,4);
    assert(r.error==-39 && r.actual==2 && r.position==4 && out[0]==3 && out[1]==4);
    r=FileTransfers::read(files,backend,ref,1,-1,out,1);
    assert(r.error==-40 && r.actual==0 && r.position==4);
    const uint8_t save[]={9,8,7};
    r=FileTransfers::write(files,backend,ref,1,6,save,3);
    assert(r.error==0 && r.actual==3 && r.position==9);
    assert((s.bytes==std::vector<uint8_t>{1,2,3,4,0,0,9,8,7}));
    uint32_t size=0;assert(FileTransfers::eof(files,ref,size)==0 && size==9);
    assert(files.close(ref)==0);assert(files.openData(id,1,false,ref)==0);
    r=FileTransfers::read(files,backend,ref,1,6,out,3);
    assert(r.error==0 && r.actual==3 && std::memcmp(out,save,3)==0);
    r=FileTransfers::write(files,backend,ref,1,0,save,3);
    assert(r.error==-61 && r.actual==0 && r.position==9);
    s.fail=true;r=FileTransfers::read(files,backend,ref,1,0,out,3);
    assert(r.error==-36 && r.actual==0 && r.position==0);
    s.fail=false;s.shortRead=true;r=FileTransfers::read(files,backend,ref,0,0,out,3);
    assert(r.error==-36 && r.actual==2 && r.position==2);
    assert(files.close(ref)==0);
    r=FileTransfers::read(files,backend,ref,0,0,out,1);assert(r.error==MacFiles::rfNumErr);
    std::puts("PASS file transfer error and distinct-write controls");
}
