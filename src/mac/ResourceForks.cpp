#include "ResourceForks.h"
#include "ResourceDirectory.h"
static int32_t residentRead(void* context,uint32_t at,uint8_t* out,uint32_t size,uint32_t& actual) {
    const uint8_t* input=(const uint8_t*)context+at;
    for(uint32_t i=0;i<size;++i)out[i]=input[i];actual=size;return 0;
}
void ResourceForks::close() {
    delete m_directory;m_directory=0;
    m_count=m_forks=0;m_open=false;
}
bool ResourceForks::appendFork(uint16_t fork,const Source& source,const uint8_t* resident) {
    if(!m_directory) { m_directory=new ResourceDirectory;if(!m_directory)return false; }
    if(m_directory->open(fork,source,false))return false;
    uint16_t count=m_directory->count(fork);
    if(count>kMaximumResources-m_count)return false;
    for(uint16_t i=0;i<count;++i) {
        ResourceDirectory::View view={};if(!m_directory->at(fork,i,view))return false;
        const auto& entry=view.entry;Record& record=m_items[m_count++];
        record.item={fork,entry.id,entry.type,entry.attrs,entry.name,entry.nameLength,resident ? resident+entry.offset : 0,entry.size};
        record.identity=view.identity;
    }
    ++m_forks;return true;
}
bool ResourceForks::finish() {
    // Retain original reference-list order for indexed Resource Manager calls.
    m_open=true;return true;
}
bool ResourceForks::open(const uint8_t* app,uint32_t appSize,const uint8_t* data,uint32_t dataSize) {
    close();Source a={(void*)app,appSize,residentRead},b={(void*)data,dataSize,residentRead};
    if(!app || !appendFork(0,a,app) || (data && !appendFork(1,b,data))) { close();return false; }
    return finish();
}
bool ResourceForks::open(const Source& app,const Source* data) {
    close();if(!appendFork(0,app) || (data && !appendFork(1,*data))) { close();return false; }
    return finish();
}
bool ResourceForks::openWithOverlay(const Source& app,const Source& overlay) {
    close();
    if(!appendFork(kOverlayFork,overlay) || !appendFork(0,app)) { close();return false; }
    return finish();
}
bool ResourceForks::item(uint32_t index,Item& out) const {
    if(!m_open || index>=m_count)return false;out=m_items[index].item;return true;
}
uint16_t ResourceForks::searchOrder(uint16_t current,uint32_t type,bool currentOnly,uint16_t* keys) const {
    if(!m_open || !keys || !m_directory->active(current))return 0;
    if(currentOnly) { keys[0]=current;return 1; }
    bool dialog=type==0x444c4f47UL || type==0x4449544cUL || type==0x414c5254UL;
    bool override=dialog && m_directory->active(kOverlayFork);
    uint16_t count=0;if(override)keys[count++]=kOverlayFork;
    int16_t key=current;
    do {
        if(!override || key!=kOverlayFork) {
            if(count==kForkCount || key<0 || key>=kForkCount)return 0;
            keys[count++]=key;
        }
    } while(m_directory->older(key,key));
    return count;
}
bool ResourceForks::find(uint16_t fork,uint32_t type,int16_t id,Item& out,uint32_t* index) const {
    if(!m_open)return false;
    for(uint16_t i=0;i<m_count;++i) {
        const Item& found=m_items[i].item;
        if(found.fork==fork && found.type==type && found.id==id) {
            out=found;if(index)*index=i;return true;
        }
    }
    return false;
}
int32_t ResourceForks::read(uint32_t index,uint8_t* out,uint32_t capacity) const {
    if(!m_open || index>=m_count || capacity<m_items[index].item.size)return -50;
    const Record& r=m_items[index];return m_directory->read(r.identity,0,out,r.item.size);
}

uint32_t ResourceForks::identity(uint32_t index) const {
    return m_open && index<m_count ? m_items[index].identity : 0;
}
bool ResourceForks::refresh(int16_t* oldToNew) {
    if(!m_directory || !oldToNew)return false;
    int16_t keys[kForkCount],key;uint16_t forks=0,total=0;
    if(m_directory->newest(key))do {
        if(key<0 || key>=kForkCount || forks==kForkCount)return false;
        keys[forks++]=key;total+=m_directory->count(key);
    } while(m_directory->older(key,key));
    if(total>kMaximumResources)return false;
    uint32_t previous[kMaximumResources];uint16_t oldCount=m_count;
    for(uint16_t i=0;i<oldCount;++i)previous[i]=m_items[i].identity;
    for(uint16_t i=0;i<kMaximumResources;++i)oldToNew[i]=-1;
    uint16_t count=0;
    // Oldest map first keeps existing map order; each map retains
    // insertion/reference order. Search order is provided by the directory.
    for(uint16_t f=forks;f;--f)for(uint16_t n=0;n<m_directory->count(keys[f-1]);++n) {
        ResourceDirectory::View view={};if(!m_directory->at(keys[f-1],n,view))return false;
        const auto& e=view.entry;
        m_items[count]={{(uint16_t)view.ref,e.id,e.type,e.attrs,e.name,e.nameLength,0,e.size},view.identity};
        for(uint16_t i=0;i<oldCount;++i)if(previous[i]==view.identity) { oldToNew[i]=count;break; }
        ++count;
    }
    m_count=count;m_forks=forks;m_open=true;return true;
}
