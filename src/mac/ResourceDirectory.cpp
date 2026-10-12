#include "ResourceDirectory.h"
#include "ResourceMap.h"
static uint32_t directoryLong(const uint8_t* p) { return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3]; }
static int32_t directoryRead(const ResourceForks::Source& source,uint32_t at,uint8_t* out,uint32_t size) {
    if(!source.read || at>source.size || size>source.size-at || (size && !out))return -50;
    while(size) {
        uint32_t n=size<ResourceForks::chunkBytes ? size : ResourceForks::chunkBytes,actual=0;
        int32_t error=source.read(source.context,at,out,n,actual);if(error)return error;if(actual!=n)return -39;
        at+=n;out+=n;size-=n;
    }
    return 0;
}
int16_t ResourceDirectory::forkIndex(int16_t ref) const { for(uint16_t i=0;i<maximumForks;++i)if(forks_[i].active && forks_[i].ref==ref)return i;return -1; }
int16_t ResourceDirectory::recordIndex(uint32_t id) const { if(id)for(uint16_t i=0;i<maximumResources;++i)if(records_[i].identity==id)return i;return -1; }
bool ResourceDirectory::active(int16_t ref) const { return forkIndex(ref)>=0; }
bool ResourceDirectory::dirty(int16_t ref) const { int16_t f=forkIndex(ref);return f>=0 && forks_[f].dirty; }
void ResourceDirectory::clear() { for(uint16_t i=0;i<maximumForks;++i)if(forks_[i].active)close(forks_[i].ref); }
int32_t ResourceDirectory::close(int16_t ref) {
    int16_t f=forkIndex(ref);if(f<0)return -193;
    for(auto& r:records_)if(r.identity && r.fork==f) { delete[] r.name;r={}; }
    delete[] forks_[f].map;forks_[f]={};return 0;
}
int32_t ResourceDirectory::parse(const ResourceForks::Source& source,uint8_t*& map,Entry*& entries,uint16_t& count) {
    map=0;entries=0;count=0;uint8_t header[16];ResourceMap::Layout layout;
    int32_t error=directoryRead(source,0,header,16);if(error)return error;
    if(!ResourceMap::layout(header,source.size,layout) || layout.mapLength>ResourceForks::maximumMapBytes)return -50;
    map=new uint8_t[layout.mapLength];if(!map)return -108;
    error=directoryRead(source,layout.mapOffset,map,layout.mapLength);
    ResourceMap parsed;
    if(!error && !parsed.open(header,source.size,map,layout.mapLength))error=-50;
    if(!error) { count=parsed.count();entries=count ? new Entry[count] : 0;if(count && !entries)error=-108; }
    for(uint16_t i=0;i<count && !error;++i) {
        ResourceMap::Entry e;uint8_t length[4];uint32_t offset;
        if(!parsed.entry(i,e)) { error=-50;break; }
        error=directoryRead(source,e.lengthOffset,length,4);if(error)break;
        uint32_t size=directoryLong(length);if(!parsed.payload(i,size,offset)) { error=-50;break; }
        entries[i]={e.type,e.id,e.attrs,e.name,e.nameLength,source,offset,size};
    }
    if(error) { delete[] map;delete[] entries;map=0;entries=0;count=0; }return error;
}
int32_t ResourceDirectory::open(int16_t ref,const ResourceForks::Source& source,bool writable) {
    if(active(ref))return -48;int16_t f=-1;uint16_t free=0;
    for(uint16_t i=0;i<maximumForks;++i)if(!forks_[i].active) { f=i;break; }
    if(f<0)return -108;for(const auto& r:records_)if(!r.identity)++free;
    uint8_t* map=0;Entry* entries=0;uint16_t count=0;int32_t error=parse(source,map,entries,count);
    if(!error && (count>free || nextIdentity_>0xffffffffUL-count-1))error=-108;
    if(error) { delete[] map;delete[] entries;return error; }
    forks_[f]={true,writable,false,ref,nextIdentity_++,map};
    uint16_t n=0;for(auto& r:records_)if(!r.identity && n<count) { r={nextIdentity_++,(uint16_t)f,entries[n++],0}; }
    delete[] entries;return 0;
}
int32_t ResourceDirectory::create(int16_t ref) {
    if(active(ref))return -48;if(nextIdentity_==0xffffffffUL)return -108;
    for(auto& f:forks_)if(!f.active) { f={true,true,true,ref,nextIdentity_++,0};return 0; }return -108;
}
bool ResourceDirectory::newest(int16_t& ref) const {
    uint32_t order=0;for(const auto& f:forks_)if(f.active && f.opened>order) { order=f.opened;ref=f.ref; }return order!=0;
}
bool ResourceDirectory::older(int16_t ref,int16_t& next) const {
    int16_t f=forkIndex(ref);if(f<0)return false;uint32_t order=0;
    for(const auto& candidate:forks_)if(candidate.active && candidate.opened<forks_[f].opened && candidate.opened>order) { order=candidate.opened;next=candidate.ref; }
    return order!=0;
}
uint16_t ResourceDirectory::count(int16_t ref) const { int16_t f=forkIndex(ref);uint16_t n=0;if(f>=0)for(const auto& r:records_)if(r.identity && r.fork==f)++n;return n; }
bool ResourceDirectory::get(uint32_t id,View& out) const { int16_t i=recordIndex(id);if(i<0)return false;const auto& r=records_[i];out={r.identity,forks_[r.fork].ref,r.entry};return true; }
uint16_t ResourceDirectory::order(int16_t fork,uint16_t* indices) const {
    uint16_t n=0;
    for(uint16_t i=0;i<maximumResources;++i)if(records_[i].identity && records_[i].fork==fork) {
        uint16_t pos=n;while(pos && records_[indices[pos-1]].identity>records_[i].identity) { indices[pos]=indices[pos-1];--pos; }
        indices[pos]=i;++n;
    }
    return n;
}
bool ResourceDirectory::at(int16_t ref,uint16_t ordinal,View& out) const {
    int16_t f=forkIndex(ref);if(f<0)return false;uint16_t indices[maximumResources];
    uint16_t n=order(f,indices);return ordinal<n && get(records_[indices[ordinal]].identity,out);
}
bool ResourceDirectory::find(int16_t ref,uint32_t type,int16_t id,View& out) const {
    int16_t f=forkIndex(ref);uint32_t first=0;
    // Slots are reused after removal; the earliest surviving insertion wins.
    if(f>=0)for(const auto& r:records_)if(r.identity && r.fork==f && r.entry.type==type && r.entry.id==id
        && (!first || r.identity<first))first=r.identity;
    return first && get(first,out);
}
uint8_t* ResourceDirectory::copyName(const Entry& e) {
    if(!e.name)return 0;uint8_t* name=new uint8_t[e.nameLength ? e.nameLength : 1];
    if(name) { volatile uint8_t* destination=name;for(uint16_t i=0;i<e.nameLength;++i)destination[i]=e.name[i]; }return name;
}
int32_t ResourceDirectory::mutation(int16_t f,int16_t replaced,const Entry* entry) const {
    if(!forks_[f].writable && entry)return -54;
    uint16_t n=count(forks_[f].ref)+(replaced<0 && entry ? 1 : 0)-(replaced>=0 && !entry ? 1 : 0);
    if(n>maximumResources)return -108;Entry* recipe=n ? new Entry[n] : 0;if(n && !recipe)return -108;
    uint16_t indices[maximumResources],total=order(f,indices),at=0;
    for(uint16_t i=0;i<total;++i) {
        if(indices[i]==replaced) { if(entry)recipe[at++]=*entry; }
        else recipe[at++]=records_[indices[i]].entry;
    }
    if(replaced<0 && entry)recipe[at++]=*entry;
    uint32_t size=0;int32_t error=ResourceWriter::measure(recipe,n,size);delete[] recipe;return error;
}
int32_t ResourceDirectory::add(int16_t ref,const Entry& entry,uint32_t& identity) {
    int16_t f=forkIndex(ref);if(f<0)return -193;
    int16_t slot=-1;for(uint16_t i=0;i<maximumResources;++i)if(!records_[i].identity) { slot=i;break; }
    if(slot<0 || nextIdentity_==0xffffffffUL)return -108;
    int32_t error=mutation(f,-1,&entry);if(error)return error;
    uint8_t* name=copyName(entry);if(entry.name && !name)return -108;
    Entry owned=entry;owned.name=name;identity=nextIdentity_++;records_[slot]={identity,(uint16_t)f,owned,name};forks_[f].dirty=true;return 0;
}
int32_t ResourceDirectory::replace(uint32_t id,const Entry& entry) {
    int16_t i=recordIndex(id);if(i<0)return -192;auto& r=records_[i];
    int32_t error=mutation(r.fork,i,&entry);if(error)return error;uint8_t* name=copyName(entry);if(entry.name && !name)return -108;
    Entry owned=entry;owned.name=name;delete[] r.name;r.name=name;r.entry=owned;forks_[r.fork].dirty=true;return 0;
}
int32_t ResourceDirectory::remove(uint32_t id) {
    int16_t i=recordIndex(id);if(i<0)return -192;auto& r=records_[i];int32_t error=mutation(r.fork,i,0);if(error)return error;
    forks_[r.fork].dirty=true;delete[] r.name;r={};return 0;
}
int32_t ResourceDirectory::read(uint32_t id,uint32_t offset,uint8_t* out,uint32_t size) const {
    View v={};if(!get(id,v))return -192;if(offset>v.entry.size || size>v.entry.size-offset)return -50;
    return directoryRead(v.entry.source,v.entry.offset+offset,out,size);
}
int32_t ResourceDirectory::publication(int16_t f,PayloadOverride select,void* context,Entry*& recipe,uint16_t* indices,uint16_t& n) const {
    uint16_t total=order(f,indices);n=0;recipe=total ? new Entry[total] : 0;if(total && !recipe)return -108;
    for(uint16_t i=0;i<total;++i) {
        const auto& r=records_[indices[i]];Entry e=r.entry;
        int32_t error=select ? select(context,r.identity,e.source,e.offset,e.size) : 0;
        if(error==omitEntry)continue;
        if(error) { delete[] recipe;recipe=0;return error; }
        recipe[n]=e;indices[n++]=indices[i];
    }
    return 0;
}
int32_t ResourceDirectory::serialize(int16_t ref,const ResourceWriter::Sink& sink,PayloadOverride select,void* context) const {
    int16_t f=forkIndex(ref);if(f<0)return -193;if(!forks_[f].writable)return -54;
    Entry* recipe=0;uint16_t indices[maximumResources],n=0;
    int32_t error=publication(f,select,context,recipe,indices,n);
    if(!error)error=ResourceWriter::serialize(recipe,n,sink);delete[] recipe;return error;
}
int32_t ResourceDirectory::rebase(int16_t ref,const ResourceForks::Source& source,PayloadOverride select,void* context) {
    int16_t f=forkIndex(ref);if(f<0)return -193;uint8_t* map=0;Entry* entries=0;uint16_t n=0;
    int32_t error=parse(source,map,entries,n);if(error)return error;
    Entry* recipe=0;uint16_t indices[maximumResources],canonical[maximumResources],total=0,position=0;
    error=publication(f,select,context,recipe,indices,total);
    for(uint16_t i=0;i<total && !error;++i) {
        uint32_t type=recipe[i].type;bool seen=false;
        for(uint16_t j=0;j<i;++j)if(recipe[j].type==type)seen=true;
        if(!seen)for(uint16_t j=i;j<total;++j)if(recipe[j].type==type)canonical[position++]=j;
    }
    if(!error && n!=total)error=-50;
    for(uint16_t i=0;i<n && !error;++i) {
        const Entry& old=recipe[canonical[i]];const Entry& e=entries[i];
        if(old.type!=e.type || old.id!=e.id || old.attrs!=e.attrs || old.size!=e.size || old.nameLength!=e.nameLength || bool(old.name)!=bool(e.name)) { error=-50;break; }
        for(uint16_t j=0;j<e.nameLength;++j)if(e.name[j]!=old.name[j]) { error=-50;break; }
    }
    // An omitted name may borrow the old map. Own it before releasing that map;
    // allocation/validation failure must leave every live record unchanged.
    uint8_t* names[maximumResources]={};bool included[maximumResources]={};bool omitted=false;
    for(uint16_t i=0;i<total && !error;++i)included[indices[i]]=true;
    for(uint16_t i=0;i<maximumResources && !error;++i) {
        const auto& r=records_[i];
        if(r.identity && r.fork==f && !included[i]) {
            omitted=true;
            if(!r.name && r.entry.name) { names[i]=copyName(r.entry);if(!names[i])error=-108; }
        }
    }
    if(!error) {
        for(uint16_t i=0;i<n;++i) { auto& r=records_[indices[canonical[i]]];delete[] r.name;r.name=0;r.entry=entries[i]; }
        for(uint16_t i=0;i<maximumResources;++i)if(names[i]) { auto& r=records_[i];r.name=names[i];r.entry.name=r.name;names[i]=0; }
        delete[] forks_[f].map;forks_[f].map=map;map=0;forks_[f].dirty=omitted;
    }
    for(auto name:names)delete[] name;
    delete[] recipe;delete[] map;delete[] entries;return error;
}
