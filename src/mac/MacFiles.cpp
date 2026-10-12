#include "MacFiles.h"
static char fold(char c) { return c>='A' && c<='Z' ? c+('a'-'A') : c; }
static bool equal(const char* a,const char* b) {
    while(*a && fold(*a)==fold(*b)) { ++a;++b; }
    return fold(*a)==fold(*b);
}
static bool copy(char* out,const char* in,uint32_t capacity) {
    if(!in)return false;
    uint32_t i=0;
    for(;in[i];++i) {
        if(i+1>=capacity || (uint8_t)in[i]<32 || (uint8_t)in[i]>=127)return false;
        out[i]=in[i];
    }
    out[i]=0;return true;
}
void MacFiles::reset() {
    fcbTableDirty_=true;
    applicationComplete=false;count_=used_=0;nextID_=2;defaultRef_=0;defaultDirectory_=0;application=system=preferences=saves=data=0;
    for(uint16_t i=0;i<maxOpen;++i)forks_[i].ref=0;
    for(uint16_t i=0;i<maxWD;++i)wd_[i].ref=0;
    add(1,"Runtime","",true); // Virtual volume root has the HFS-reserved ID 2.
}
const MacFiles::Entry* MacFiles::entry(uint32_t id) const {
    if(id<2)return 0;
    for(uint16_t i=0;i<used_;++i)if(entries_[i].id==id)return &entries_[i];
    return 0;
}
const MacFiles::Entry* MacFiles::child(uint32_t parent,const char* name) const {
    for(uint16_t i=0;i<used_;++i)
        if(entries_[i].id && entries_[i].parent==parent && equal(entries_[i].name,name))return &entries_[i];
    return 0;
}
int32_t MacFiles::add(uint32_t parent,const char* name,const char* path,bool directory,uint32_t ds,uint32_t rs,bool resourceIsBase) {
    if(count_==maxEntries || nextID_>0x7fffffffUL || !name || !*name)return unsupported;
    if(count_ && (!entry(parent) || !entry(parent)->directory || child(parent,name)))return paramErr;
    Entry candidate={};
    if(!copy(candidate.name,name,sizeof(candidate.name)) || !copy(candidate.path,path,sizeof(candidate.path)))return unsupported;
    for(const char* p=name;*p;++p)if(*p==':')return paramErr;
    candidate.id=nextID_;candidate.parent=parent;candidate.directory=directory;
    candidate.dataSize=ds;candidate.resourceSize=rs;candidate.resourceIsBase=resourceIsBase;
    uint16_t slot=0;while(slot<used_ && entries_[slot].id)++slot;
    entries_[slot]=candidate;if(slot==used_)++used_;++count_;++nextID_;return candidate.id;
}
// Measured HFS ordering for supported printable ASCII. Case folds to capitals;
// grave accent lies between A and B, rather than at its ASCII position.
static uint16_t weight(char c) {
    if(c>='a' && c<='z')c-='a'-'A';
    return c=='`' ? ('A'*2+1) : (uint8_t)c*2;
}
static bool precedes(const char* a,const char* b) {
    while(*a && weight(*a)==weight(*b)) { ++a;++b; }
    return weight(*a)<weight(*b);
}
int16_t MacFiles::indexedFile(int16_t volume,uint32_t directory,int16_t index,uint32_t& id) const {
    if(index<=0)return unsupported;
    uint32_t parent=0;int16_t error=resolve(volume,directory,0,parent);
    if(error)return error==dirNFErr ? fnfErr : error;
    // Legacy application and System/root namespaces remain intentionally partial.
    // Never turn an incomplete enumeration into a false end-of-directory result.
    if(parent==2 || (parent==application && !applicationComplete) || parent==system)return unsupported;
    for(uint16_t i=0;i<used_;++i) {
        const Entry& candidate=entries_[i];
        if(!candidate.id || candidate.directory || candidate.parent!=parent)continue;
        uint16_t rank=1;
        for(uint16_t j=0;j<used_;++j) {
            const Entry& other=entries_[j];
            if(other.id && !other.directory && other.parent==parent && precedes(other.name,candidate.name))++rank;
        }
        if(rank==(uint16_t)index) { id=candidate.id;return noErr; }
    }
    return fnfErr;
}
int16_t MacFiles::forkPath(uint32_t id,bool resource,char* path,uint32_t capacity) const {
    const Entry* e=entry(id);if(!e || e->directory)return fnfErr;
    const char* suffix=e->resourceIsBase ? (resource ? "" : ".data") : (resource ? ".rsrc" : "");
    uint32_t n=0;while(e->path[n])++n;
    uint32_t extra=0;while(suffix[extra])++extra;
    if(!path || n+extra>=capacity)return unsupported;
    for(uint32_t i=0;i<n;++i)path[i]=e->path[i];
    for(uint32_t i=0;i<=extra;++i)path[n+i]=suffix[i];
    return noErr;
}
int16_t MacFiles::planCreate(int16_t volume,uint32_t directory,const char* path,Entry& candidate) const {
    if(!path)return unsupported; // Null ioNamePtr has not been measured for Create.
    if(!*path) { uint32_t existing=0;int16_t error=resolve(volume,directory,0,existing);return error ? error : -48; }
    const char* leaf=path;const char* last=0;
    for(const char* p=path;*p;++p)if(*p==':')last=p;
    uint32_t parent=0;
    if(last) {
        if(!last[1])return unsupported; // Directory-shaped Create needs reference evidence.
        char prefix[256];uint16_t n=0;
        for(const char* p=path;p<=last;++p) { if(n==255)return unsupported;prefix[n++]=*p; }
        prefix[n]=0;bool full=*path!=':';
        int16_t error=resolve(full ? volumeRef : volume,full ? 0 : directory,prefix,parent);
        if(error)return error;
        leaf=last+1;
    } else {
        int16_t error=resolve(volume,directory,0,parent);if(error)return error;
    }
    if(child(parent,leaf))return -48;
    if(count_==maxEntries || nextID_>0x7fffffffUL)return unsupported;
    Entry planned={};planned.parent=parent;
    if(!copy(planned.name,leaf,sizeof(planned.name)))return unsupported;
    // Mac slashes cannot pass through as native DOS path separators. Reserved
    // companion suffixes need the explicit fork/metadata storage layer.
    const char* suffix=0;
    for(const char* p=leaf;*p;++p) {
        if(*p=='/')return unsupported;
        if(*p=='.') {
            if(equal(p,".finfo.new") || equal(p,".finfo.old"))return unsupported;
            suffix=p;
        }
    }
    if(equal(leaf,".") || equal(leaf,"..") || (suffix && (equal(suffix,".rsrc") || equal(suffix,".finfo"))))return unsupported;
    const char* folder=entry(parent)->path;if(!*folder)return unsupported;
    uint16_t n=0;
    while(*folder) { if(n==158)return unsupported;planned.path[n++]=*folder++; }
    if(planned.path[n-1]!=':' && planned.path[n-1]!='/')planned.path[n++]='/';
    for(const char* p=leaf;*p;++p) { if(n==159)return unsupported;planned.path[n++]=*p; }
    planned.path[n]=0;candidate=planned;return noErr;
}
int16_t MacFiles::setMetadata(uint32_t id,const FileMetadata::Record& metadata,bool dirty) {
    fcbTableDirty_=true;
    Entry* e=const_cast<Entry*>(entry(id));if(!e)return fnfErr;
    if(e->directory)return unsupported;
    e->metadata=metadata;e->metadataKnown=true;e->metadataDirty=dirty;return noErr;
}
void MacFiles::touchMetadata(uint32_t id,uint32_t date) {
    Entry* e=const_cast<Entry*>(entry(id));if(e && e->metadataKnown) { e->metadata.modified=date;e->metadataDirty=true; }
}
void MacFiles::metadataFlushed(uint32_t id) {
    Entry* e=const_cast<Entry*>(entry(id));if(e)e->metadataDirty=false;
}
int16_t MacFiles::canRemove(uint32_t id) const {
    const Entry* e=entry(id);if(!e)return fnfErr;
    for(uint16_t i=0;i<maxOpen;++i)if(forks_[i].ref && forks_[i].id==id)return -47;
    if(e->directory) {
        for(uint16_t i=0;i<used_;++i)if(entries_[i].id && entries_[i].parent==id)return -47;
        for(uint16_t i=0;i<maxWD;++i)if(wd_[i].ref && wd_[i].directory==id)return -47;
        return unsupported; // Virtual/native directory lifetime is not implemented.
    }
    return noErr;
}
int16_t MacFiles::remove(uint32_t id) {
    int16_t error=canRemove(id);if(error)return error;
    Entry* e=const_cast<Entry*>(entry(id));e->id=0;--count_;return noErr;
}
int16_t MacFiles::directoryFor(int16_t ref,uint32_t& directory) const {
    if(!ref) {
        directory=defaultDirectory_ ? defaultDirectory_ : application;
        return directory ? noErr : dirNFErr;
    }
    if(ref==applicationWD && application) { directory=application;return noErr; }
    if(ref==volumeRef) { directory=2;return noErr; }
    for(uint16_t i=0;i<maxWD;++i)if(wd_[i].ref==ref) { directory=wd_[i].directory;return noErr; }
    return nsvErr;
}
int16_t MacFiles::resolve(int16_t volume,uint32_t directory,const char* path,uint32_t& id,bool fileOpen) const {
    uint32_t base;
    if(directoryFor(volume,base))return nsvErr;
    if(directory)base=directory;
    if(!entry(base) || !entry(base)->directory)return fileOpen ? fnfErr : dirNFErr;
    if(!path || !*path) { id=base;return noErr; }
    const char* p=path;
    bool absolute=false;
    if(*p==':')++p;
    else { for(const char* q=p;*q;++q)if(*q==':') { absolute=true;break; } }
    if(absolute) {
        char volumeName[32];uint16_t n=0;
        while(*p && *p!=':') { if(n==31 || (uint8_t)*p>=128)return unsupported;volumeName[n++]=*p++; }
        volumeName[n]=0;
        if(!equal(volumeName,entry(2)->name))return nsvErr;
        base=2;++p;
    }
    while(*p) {
        if(*p==':') {
            if(base==2)return dirNFErr;
            base=entry(base)->parent;++p;continue;
        }
        char component[32];uint16_t n=0;
        while(*p && *p!=':') {
            if(n==31 || (uint8_t)*p>=128)return unsupported;
            component[n++]=*p++;
        }
        component[n]=0;
        const Entry* found=child(base,component);
        if(!found) {
            // Partial namespaces cannot establish absence beyond the measured optional Movies path.
            if(base==2 || base==system || (base==application && !applicationComplete))return unsupported;
            return fileOpen && *p==':' && p[1] ? dirNFErr : fnfErr;
        }
        base=found->id;
        if(*p==':') { if(!found->directory)return fileOpen ? fnfErr : dirNFErr;++p; }
    }
    id=base;return noErr;
}
int16_t MacFiles::open(uint32_t id,bool resource,bool writable) {
    fcbTableDirty_=true;
    if(!entry(id) || entry(id)->directory)return fnfErr;
    for(uint16_t i=0;i<maxOpen;++i)if(!forks_[i].ref) {
        forks_[i]={(int16_t)(2+i*fcbLength),id,0,resource,writable,false,false,false};return forks_[i].ref;
    }
    return -42; // tmfoErr
}
int16_t MacFiles::openFork(uint32_t id,bool resource,uint8_t permission,bool locked,int16_t& ref) {
    ref=0;
    if(permission>4)return unsupported;
    if(!entry(id) || entry(id)->directory)return fnfErr;
    if(locked && permission>1)return -54; // permErr, measured local HFS Open.
    bool writable=permission!=1 && !locked;
    if(writable)for(uint16_t i=0;i<maxOpen;++i) {
        const Fork& f=forks_[i];
        if(f.ref && f.id==id && f.resource==resource && f.writable && !(permission==4 && f.shared)) {
            ref=f.ref;return -49; // The failed Open returns the existing writer ref.
        }
    }
    int16_t result=open(id,resource,writable);
    if(result<0)return result;
    ref=result;Fork* f=const_cast<Fork*>(fork(ref));f->shared=permission==4;f->locked=locked;
    return noErr;
}
int16_t MacFiles::volumeParameters(int16_t ref,const char* name,uint8_t* buffer,uint32_t requested,uint32_t& actual) const {
    if(requested>0x7fffffffUL || (!buffer && requested))return unsupported;
    int16_t error=volume(ref,name);if(error)return error;
    // Measured System 7.5.5 local HFS record: version 2, attributes $10E0,
    // no shared-volume handle/server, unrated speed, standard HFS privileges.
    actual=requested<20 ? requested : 20;
    for(uint32_t i=0;i<actual;++i)buffer[i]=i==1 ? 2 : i==4 ? 0x10 : i==5 ? 0xe0 : 0;
    return noErr;
}
int16_t MacFiles::volume(int16_t ref,const char* name) const {
    // A full pathname's volume prefix overrides the reference. A bare name
    // is ignored by the reference FlushVol; ref zero then selects the default.
    if(name && *name!=':')for(const char* end=name;*end;++end)if(*end==':') {
        const char* expected=entry(2)->name;const char* p=name;
        while(p<end && *expected && fold(*p)==fold(*expected)) { ++p;++expected; }
        return p==end && !*expected ? noErr : nsvErr;
    }
    if(ref==1)return noErr; // Single native volume is virtual drive 1.
    uint32_t directory=0;return directoryFor(ref,directory);
}
int16_t MacFiles::selectVolume(int16_t& ref,int16_t index,const char* name,uint32_t& directory) const {
    if(index>0) {
        if(index!=1) { ref=0;return nsvErr; }
        directory=2;ref=volumeRef;return noErr;
    }
    int16_t error=volume(ref,index<0 ? name : 0);if(error)return error;
    bool absolute=false;
    if(index<0 && name && *name!=':')for(const char* p=name;*p;++p)if(*p==':')absolute=true;
    if(absolute || ref==1)directory=2;
    else { error=directoryFor(ref,directory);if(error)return error; }
    ref=volumeRef;return noErr;
}
static void volumeWord(uint8_t* pb,uint16_t offset,uint16_t value) { pb[offset]=value>>8;pb[offset+1]=value; }
static void volumeLong(uint8_t* pb,uint16_t offset,uint32_t value) { volumeWord(pb,offset,value>>16);volumeWord(pb,offset+2,value); }
int16_t MacFiles::volumeInfo(uint32_t directory,const MacVolumeBacking& backing,uint8_t* pb) const {
    if(!pb || !entry(directory) || !entry(directory)->directory || !entry(system)
        || !backing.blocks || backing.used>backing.blocks || !backing.blockBytes)return unsupported;
    uint32_t groups=1,blockBytes=backing.blockBytes;
    // Both HFS counts are 16-bit. Aggregate whole native blocks, rounding
    // capacity/free space down so the virtual volume never overstates either.
    while(backing.blocks/groups>65535) {
        if(blockBytes>0x7fffffffUL || groups>0x7fffffffUL)return unsupported;
        blockBytes*=2;groups*=2;
    }
    uint32_t files=0,directories=0,valence=0;
    for(uint16_t i=0;i<used_;++i) {
        const Entry& e=entries_[i];if(!e.id || e.id==2)continue;
        if(e.directory)++directories;else ++files;
        if(e.parent==directory && (directory!=2 || !e.directory))++valence;
    }
    for(uint16_t i=30;i<122;++i)pb[i]=0;
    volumeLong(pb,30,backing.created);volumeLong(pb,34,backing.modified);
    volumeWord(pb,38,backing.locked ? 0x80 : 0);volumeWord(pb,40,valence);
    volumeWord(pb,46,backing.blocks/groups);volumeLong(pb,48,blockBytes);
    volumeLong(pb,52,blockBytes);volumeLong(pb,58,nextID_);
    volumeWord(pb,62,(backing.blocks-backing.used)/groups);
    volumeWord(pb,64,0x4244);volumeWord(pb,66,1);volumeWord(pb,68,0xffff);
    volumeLong(pb,82,files);volumeLong(pb,86,directories);
    volumeLong(pb,90,system);volumeLong(pb,98,application);
    return noErr;
}
void MacFiles::modified(int16_t ref) {
    fcbTableDirty_=true;
    Fork* f=const_cast<Fork*>(fork(ref));if(f)f->modified=true;
}
void MacFiles::flushed(uint32_t id,bool resource) {
    fcbTableDirty_=true;
    for(uint16_t i=0;i<maxOpen;++i)if(forks_[i].ref && forks_[i].id==id && forks_[i].resource==resource)forks_[i].modified=false;
}
const MacFiles::Fork* MacFiles::fork(int16_t ref) const {
    for(uint16_t i=0;i<maxOpen;++i)if(forks_[i].ref && forks_[i].ref==ref)return &forks_[i];
    return 0;
}
int16_t MacFiles::queryFork(int16_t volume,int16_t index,int16_t ref,const Fork*& found) const {
    found=0;
    if(index<0)return unsupported;
    if(!index) { found=fork(ref);return found ? noErr : rfNumErr; }
    uint32_t directory=0;
    if(volume && volume!=1 && directoryFor(volume,directory))return nsvErr;
    // Single catalogued volume, drive 1. Index only live forks, not table holes.
    for(uint16_t i=0;i<maxOpen;++i)if(forks_[i].ref && !--index) {
        found=&forks_[i];return noErr;
    }
    return -38; // fnOpnErr, measured on System 7.5.5 for an exhausted index.
}
int16_t MacFiles::close(int16_t ref) {
    fcbTableDirty_=true;
    for(uint16_t i=0;i<maxOpen;++i)if(forks_[i].ref && forks_[i].ref==ref) { forks_[i].ref=0;return noErr; }
    return rfNumErr;
}
int16_t MacFiles::initializeDirectories() {
    if(!entry(application) || !entry(system) || !entry(application)->directory || !entry(system)->directory)return dirNFErr;
    for(uint16_t i=0;i<maxWD;++i)if(wd_[i].ref)return unsupported;
    // SysEnvirons reports this reference; System 7.5.5 identifies its owner as ERIK.
    wd_[0]={systemWD,system,0x4552494b};return noErr;
}
int16_t MacFiles::queryWD(int16_t& ref,int16_t index,uint32_t& process,uint32_t& directory) const {
    if(index<=0) {
        int16_t actual=ref ? ref : volumeRef;
        uint32_t found=0;
        int16_t error=directoryFor(actual,found);
        if(error)return error;
        ref=actual;directory=found;process=wdProcess(actual);return noErr;
    }
    uint32_t ignored=0;
    if(ref && directoryFor(ref,ignored))return nsvErr;
    if(application && !process && !--index) {
        ref=applicationWD;directory=application;process=0;return noErr;
    }
    for(uint16_t i=0;i<maxWD;++i)if(wd_[i].ref && (!process || wd_[i].process==process) && !--index) {
        ref=wd_[i].ref;process=wd_[i].process;directory=wd_[i].directory;return noErr;
    }
    return nsvErr;
}
int16_t MacFiles::openWD(uint32_t directory,uint32_t process,bool* created) {
    if(created)*created=false;
    if(!entry(directory) || !entry(directory)->directory)return fnfErr;
    if(directory==application)return applicationWD;
    if(directory==2)return volumeRef;
    for(uint16_t i=0;i<maxWD;++i)
        if(wd_[i].ref && wd_[i].directory==directory)return wd_[i].ref;
    for(uint16_t i=0;i<maxWD;++i)if(!wd_[i].ref) {
        wd_[i]={(int16_t)(-31999+i),directory,process};if(created)*created=true;return wd_[i].ref;
    }
    return -121; // tmwdoErr
}
int16_t MacFiles::closeWD(int16_t ref) {
    if(ref==volumeRef || (ref==applicationWD && application))return noErr;
    for(uint16_t i=0;i<maxWD;++i)if(wd_[i].ref && wd_[i].ref==ref) { wd_[i].ref=0;return noErr; }
    return rfNumErr;
}
uint32_t MacFiles::wdProcess(int16_t ref) const {
    for(uint16_t i=0;i<maxWD;++i)if(wd_[i].ref && wd_[i].ref==ref)return wd_[i].process;
    return 0;
}

int16_t MacFiles::setDefault(int16_t ref,const char* volumeName) {
    if(volumeName && !equal(volumeName,entry(2)->name))return unsupported;
    uint32_t directory=0;
    int16_t error=directoryFor(ref,directory);
    if(error)return error;
    if(ref)defaultRef_=ref;
    defaultDirectory_=directory;
    return noErr;
}

int16_t MacFiles::setHierarchicalDefault(int16_t ref,uint32_t directory,const char* path) {
    // Full paths select their own volume/root, including with an invalid input ref.
    bool absolute=false;
    if(path && *path!=':')for(const char* p=path;*p;++p)if(*p==':')absolute=true;
    uint32_t resolved=0;
    int16_t error=resolve(absolute ? volumeRef : ref,absolute ? 2 : directory,path,resolved);
    if(error==dirNFErr)return fnfErr; // HSetVol's missing-directory result on 7.5.5.
    if(error)return error;
    if(!entry(resolved)->directory)return fnfErr;
    defaultDirectory_=resolved;defaultRef_=volumeRef;
    return noErr;
}

int16_t MacFiles::seek(int16_t ref,uint16_t mode,int32_t offset,bool writing) {
    fcbTableDirty_=true;
    Fork* f=const_cast<Fork*>(fork(ref));
    if(!f)return rfNumErr;
    if(mode>3)return unsupported;
    if(!mode)return noErr;
    const Entry* e=entry(f->id);
    uint32_t size=f->resource ? e->resourceSize : e->dataSize;
    uint32_t base=mode==1 ? 0 : mode==2 ? size : f->position;
    if(offset<0) {
        uint32_t magnitude=0-(uint32_t)offset;
        if(base<magnitude)return -40; // posErr: unchanged mark.
        base-=magnitude;
    } else {
        if((uint32_t)offset>0x7fffffffUL-base) { if(writing)return paramErr;f->position=size;return -39; }
        base+=(uint32_t)offset;
    }
    f->position=!writing && base>size ? size : base;
    return !writing && base>size ? -39 : noErr;
}
int16_t MacFiles::setSize(int16_t ref,uint32_t size,bool clampPosition) {
    fcbTableDirty_=true;
    Fork* f=const_cast<Fork*>(fork(ref));
    if(!f)return rfNumErr;
    if(!f->writable)return -61;
    if(size>0x7fffffffUL)return paramErr;
    Entry* e=const_cast<Entry*>(entry(f->id));
    if(size!=(f->resource ? e->resourceSize : e->dataSize))f->modified=true;
    if(f->resource)e->resourceSize=size;else e->dataSize=size;
    if(clampPosition && f->position>size)f->position=size;
    return noErr;
}
void MacFiles::advance(int16_t ref,uint32_t count) {
    fcbTableDirty_=true;
    Fork* f=const_cast<Fork*>(fork(ref));
    if(f)f->position+=count; // Caller supplies the actual bounded transfer count.
}

// Inside Macintosh: Files, FCBRec. File references are byte offsets into
// the table, following its two-byte length header; records must not overlap.
void MacFiles::writeFCBTable(uint8_t* table,uint32_t volumeControlBlock) {
    fcbTableDirty_=false;
    for(uint16_t i=0;i<fcbTableSize;++i)table[i]=0;
    auto word=[](uint8_t* p,uint16_t v) { p[0]=v>>8;p[1]=v; };
    auto lng=[word](uint8_t* p,uint32_t v) { word(p,v>>16);word(p+2,v); };
    word(table,fcbTableSize);
    for(uint16_t i=0;i<maxOpen;++i) {
        const Fork& f=forks_[i];if(!f.ref)continue;
        const Entry* file=entry(f.id);uint8_t* p=table+f.ref;
        lng(p,f.id);
        p[4]=(f.writable?1:0)|(f.resource?2:0)|(f.shared?16:0)
            |(f.locked?32:0)|(f.modified?128:0);
        uint32_t length=f.resource?file->resourceSize:file->dataSize;
        lng(p+8,length);lng(p+12,length);lng(p+16,f.position);
        lng(p+20,volumeControlBlock);
        for(uint16_t j=0;j<4;++j)p[50+j]=file->metadata.finder[j];
        lng(p+58,file->parent);
        uint8_t n=0;while(file->name[n]) {p[63+n]=file->name[n];++n;}p[62]=n;
    }
}
