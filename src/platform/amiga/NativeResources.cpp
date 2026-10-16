#ifdef POP2_CORE
#include "NativeResources.h"
#include "NativeFiles.h"
#include "mac/FileReadCache.h"
#include "mac/ResourceStage.h"
namespace {
ResourceDirectory* directory=0;
struct Fork { int16_t ref;uint32_t id;const uint8_t* resident;uint32_t size;uint8_t* buffer=0;FileReadCache cache; } forks[16]={};
int16_t selected=-1;
int16_t keyFor(int16_t ref) { for(int16_t i=0;i<16;++i)if(forks[i].ref==ref && ref)return i;return -1; }
int32_t backendRead(void* context,uint32_t at,uint8_t* out,uint32_t count,uint32_t& actual) {
    Fork& fork=*(Fork*)context;const auto& backend=NativeFiles::backend();
    return backend.read(backend.context,fork.id,true,at,out,count,actual);
}
int32_t sourceRead(void* context,uint32_t at,uint8_t* out,uint32_t count,uint32_t& actual) {
    Fork& fork=*(Fork*)context;actual=0;
    if(at>fork.size || count>fork.size-at)return -50;
    if(fork.resident) {for(uint32_t i=0;i<count;++i)out[i]=fork.resident[at+i];actual=count;return 0;}
    return fork.cache.read(at,out,count,actual);
}
}
namespace NativeResources {
bool prepare(const uint8_t* app,uint32_t size) {
    release();directory=new ResourceDirectory;if(!directory)return false;
    auto& files=NativeFiles::catalog();uint32_t id=0;
    if(files.resolve(0,0,"Prince of Persia 2",id,true))return false;
    int16_t ref=0;if(files.openFork(id,true,1,false,ref))return false;
    forks[0]={ref,id,app,size};ResourceForks::Source source={forks,size,sourceRead};
    if(directory->open(0,source,false))return false;selected=0;return true;
}
void release() {
    delete directory;directory=0;
    for(auto& fork:forks) {delete[] fork.buffer;if(fork.ref)NativeFiles::catalog().close(fork.ref);fork={};}
    selected=-1;
}
bool dirty(int16_t ref) {int16_t key=keyFor(ref);return key>=0 && directory && directory->dirty(key);}
int16_t application() {return forks[0].ref;}
int16_t newest() {int16_t key=-1;return directory && directory->newest(key) && key>=0 && key<16 ? forks[key].ref : -1;}
int16_t current() {return selected<0 ? -1 : forks[selected].ref;}
int32_t use(int16_t ref) {
    int16_t key=keyFor(ref);if(key<0)return -193;selected=key;return 0;
}
int32_t open(int16_t volume,uint32_t folder,const char* path,uint8_t permission,int16_t& ref) {
    auto& files=NativeFiles::catalog();uint32_t id=0;ref=-1;
    int32_t error=files.resolve(volume,folder,path,id,true);if(error)return error;
    for(int16_t key=0;key<16;++key)if(forks[key].ref && forks[key].id==id) {
        selected=key;ref=forks[key].ref;return 0;
    }
    int16_t key=0;while(key<16 && forks[key].ref)++key;if(key==16)return -42;
    int16_t opened=0;error=NativeFiles::openFork(volume,folder,path,true,permission,opened);if(error)return error;
    forks[key]={opened,id,0,files.entry(id)->resourceSize};
    forks[key].buffer=new uint8_t[FileReadCache::capacity];
    if(!forks[key].buffer) {files.close(opened);forks[key]={};return -108;}
    forks[key].cache.bind(forks[key].buffer,forks[key].size,backendRead,forks+key);
    ResourceForks::Source source={forks+key,forks[key].size,sourceRead};
    const auto* f=files.fork(opened);error=directory->open(key,source,f->writable);
    if(error) {files.close(opened);delete[] forks[key].buffer;forks[key]={};return error;}
    selected=key;ref=opened;return 0;
}
// Publication and rebasing follow Alone in the Dark's ResourceFiles adapter.
int32_t publish(int16_t ref,ResourceDirectory::PayloadOverride select,void* context) {
    int16_t key=keyFor(ref);if(key<0)return -193;
    auto& files=NativeFiles::catalog();const auto* f=files.fork(ref);
    if(!f || !f->writable)return -61;
    if(key==0 || !NativeFiles::mutableResource(f->id))return MacFiles::unsupported;
    // Staging owns its paths on the heap, outside the small trap stack.
    struct Publication {
        ResourceStage stage;uint32_t size=0;
        static int32_t begin(void* p,uint32_t size) {
            auto& r=*(Publication*)p;r.size=size;auto sink=r.stage.sink();return sink.begin(sink.context,size);
        }
        static int32_t write(void* p,uint32_t at,const uint8_t* data,uint32_t size,uint32_t& actual) {
            auto sink=((Publication*)p)->stage.sink();return sink.write(sink.context,at,data,size,actual);
        }
        static int32_t finish(void* p,bool publish) {
            auto sink=((Publication*)p)->stage.sink();return sink.finish(sink.context,publish);
        }
    };
    auto* publication=new Publication;if(!publication)return -108;
    char path[192];int32_t error=files.forkPath(f->id,true,path,sizeof(path));
    if(!error && !publication->stage.bind(path))error=MacFiles::unsupported;
    ResourceWriter::Sink sink={publication,Publication::begin,Publication::write,Publication::finish};
    if(!error)error=directory->serialize(key,sink,select,context);
    uint32_t size=publication->size;delete publication;if(error)return error;
    error=NativeFiles::resourcePublished(ref,size);if(error)return error;
    Fork& fork=forks[key];fork.size=size;fork.cache.bind(fork.buffer,size,backendRead,&fork);
    ResourceForks::Source source={&fork,size,sourceRead};
    return directory->rebase(key,source,select,context);
}
int32_t close(int16_t ref) {
    int16_t key=keyFor(ref);if(key<0)return -193;
    if(key==0 || directory->dirty(key))return MacFiles::unsupported;
    int16_t older=-1;if(selected==key && !directory->older(key,older))return MacFiles::unsupported;
    int32_t error=directory->close(key);if(error)return error;
    error=NativeFiles::catalog().close(ref);delete[] forks[key].buffer;forks[key]={};
    if(selected==key)selected=older;
    return error;
}
int32_t find(uint32_t type,int16_t id,bool currentOnly,Item& out) {
    if(!directory || selected<0)return -193;
    int16_t key=selected;ResourceDirectory::View view={};
    do {
#ifdef POP2_AUDIO
        if(key==0 && type==0x4a6e7468 && id==11){out={0xfffffff0UL,type,4,id,forks[0].ref,0};return 0;}
#endif
        if(directory->find(key,type,id,view)) {
            out={view.identity,view.entry.type,view.entry.size,view.entry.id,forks[key].ref,view.entry.attrs};return 0;
        }
        if(currentOnly)break;
    }while(directory->older(key,key));
    return -192;
}
int32_t info(uint32_t identity,Item& out,const uint8_t*& name,uint8_t& nameLength) {
    if(!directory)return -193;
#ifdef POP2_AUDIO
    if(identity==0xfffffff0UL){static const uint8_t label[]="Paula Halestorm";out={identity,0x4a6e7468,4,11,forks[0].ref,0};name=label;nameLength=sizeof(label)-1;return 0;}
#endif
    ResourceDirectory::View view={};if(!directory->get(identity,view))return -192;
    out={identity,view.entry.type,view.entry.size,view.entry.id,forks[view.ref].ref,view.entry.attrs};
    name=view.entry.name;nameLength=view.entry.nameLength;return 0;
}
// Same shared mutable directory used by Alone in the Dark's ResourceFiles adapter.
int32_t add(const ResourceWriter::Entry& entry,Item& out) {
    if(!directory || selected<0)return -193;
    uint32_t identity=0;int32_t error=directory->add(selected,entry,identity);
    if(error)return error;
    out={identity,entry.type,entry.size,entry.id,forks[selected].ref,entry.attrs};return 0;
}
int32_t remove(uint32_t identity) {
    return directory ? directory->remove(identity) : -193;
}
int32_t read(uint32_t identity,uint8_t* bytes,uint32_t size) {
    if(!directory)return -193;
#ifdef POP2_AUDIO
    if(identity==0xfffffff0UL){if(size!=4)return -50;bytes[0]=0xaf;bytes[1]=0xfc;bytes[2]=0x4e;bytes[3]=0x75;return 0;}
#endif
    ResourceDirectory::View view={};
    if(!directory->get(identity,view))return -192;
    if(size!=view.entry.size)return -50;
    return directory->read(identity,0,bytes,size);
}
}
#endif
