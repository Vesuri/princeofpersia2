// Replay verified original File Manager transfers through the portable service.
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <stdexcept>
#include "mac/FileTransfers.h"
static void require(bool yes,const char* message) { if(!yes)throw std::runtime_error(message); }
struct Storage { MacFiles files;std::map<uint32_t,std::string> paths; };
static int32_t read(void* context,uint32_t id,bool resource,uint32_t offset,uint8_t* out,uint32_t count,uint32_t& actual) {
    require(!resource,"unexpected resource fork");Storage& s=*(Storage*)context;
    std::ifstream f(s.paths.at(id),std::ios::binary);if(!f)return -43;
    f.seekg(offset);f.read((char*)out,count);actual=(uint32_t)f.gcount();return f.bad() ? -36 : 0;
}
static int32_t write(void* context,uint32_t id,bool resource,uint32_t offset,const uint8_t* in,uint32_t count,uint32_t& actual) {
    require(!resource,"unexpected resource fork");Storage& s=*(Storage*)context;
    std::fstream f(s.paths.at(id),std::ios::in|std::ios::out|std::ios::binary);if(!f)return -43;
    f.seekp(offset);f.write((const char*)in,count);f.flush();if(!f)return -36;actual=count;return 0;
}
static std::vector<uint8_t> load(const std::string& path) {
    std::ifstream f(path,std::ios::binary);require(bool(f),"fixture payload missing");
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(f),{});
}
int main(int argc,char** argv) {
    try {
        require(argc==2,"fixture script argument");std::ifstream script(argv[1]);require(bool(script),"fixture missing");
        Storage storage;storage.files.reset();FileTransfers::Backend backend={&storage,read,write};
        std::map<int,int16_t> refs;std::map<std::string,uint32_t> ids;
        char op;unsigned operations=0,reads=0,writes=0;
        while(script>>op) {
            int ref;script>>ref;require(bool(script),"fixture ref");
            if(op=='O') {
                std::string path;unsigned permission;script>>std::quoted(path)>>permission;
                uint32_t id;
                if(ids.count(path))id=ids.at(path);
                else {
                    auto bytes=load(path);std::string name="file"+std::to_string(ids.size());
                    int32_t added=storage.files.add(2,name.c_str(),name.c_str(),false,bytes.size());
                    require(added>=2,"catalog add");id=added;ids[path]=id;storage.paths[id]=path;
                }
                int16_t opened=0;require(storage.files.openData(id,permission,false,opened)==0,"open");refs[ref]=opened;
            } else if(op=='C') {
                require(storage.files.close(refs.at(ref))==0,"close");refs.erase(ref);
            } else if(op=='E') {
                uint32_t expected,size=0;script>>expected;
                require(FileTransfers::eof(storage.files,refs.at(ref),size)==0 && size==expected,"EOF mismatch");
            } else if(op=='S') {
                unsigned mode,position;int32_t offset;script>>mode>>offset>>position;
                require(storage.files.seek(refs.at(ref),mode,offset)==0,"seek error");
                require(storage.files.fork(refs.at(ref))->position==position,"seek position");
            } else {
                require(op=='R'||op=='W',"fixture operation");
                unsigned mode,count,actual,position;int32_t offset,error;std::string payload;
                script>>mode>>offset>>count>>actual>>error>>position>>std::quoted(payload);
                auto expected=load(payload);std::vector<uint8_t> buffer(count);
                FileTransfers::Result result;
                if(op=='W') {require(expected.size()==count,"write request");result=FileTransfers::write(storage.files,backend,refs.at(ref),mode,offset,expected.data(),count);++writes;}
                else {result=FileTransfers::read(storage.files,backend,refs.at(ref),mode,offset,buffer.data(),count);buffer.resize(result.actual);require(buffer==expected,"read payload");++reads;}
                require(result.error==error && result.actual==actual && result.position==position,"transfer result");
            }
            require(bool(script),"fixture fields");++operations;
        }
        require(script.eof() && reads>0 && writes>0,"fixture completion");
        std::cout<<"FILE TRANSFER REPLAY PASS operations="<<operations<<" reads="<<reads<<" writes="<<writes<<"\n";
        return 0;
    }catch(const std::exception& e) {std::cerr<<"FILE TRANSFER REPLAY / "<<e.what()<<"\n";return 1;}
}
