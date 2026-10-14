#ifdef POP2_CORE
#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/memory.h>
#include <dos/dosextens.h>
#include "NativeFiles.h"
#include "SystemWindow.h"
#include "mac/SHA256.h"
#include "../../../build/core/Inputs.h"
extern DosLibrary* DOSBase;
extern "C" { volatile uint32_t g_fileVerifiedFiles=0,g_fileVerifiedBytes=0,g_fileVolume[128][6]={},g_fileVolumeQueries=0; }
namespace {
MacFiles files;
struct Source { uint32_t id,size[2];uint8_t hash[2][32];bool verified[2],settings; };
Source sources[MacFiles::maxEntries]={};
uint8_t scratch[65536],catalogBytes[8192];
uint16_t word(const uint8_t* p) { return uint16_t(p[0])<<8|p[1]; }
void putWord(uint8_t* p,uint16_t v) { p[0]=v>>8;p[1]=v; }
void putLong(uint8_t* p,uint32_t v) { putWord(p,v>>16);putWord(p+2,v); }
uint32_t lng(const uint8_t* p) { return uint32_t(word(p))<<16|word(p+2); }
bool equal(const uint8_t* a,const uint8_t* b,uint32_t n) { while(n--)if(*a++!=*b++)return false;return true; }
bool suffix(char* out,const char* path,const char* ending) {
    unsigned n=0;while(*path) {if(n==191)return false;out[n++]=*path++;}
    while(*ending) {if(n==191)return false;out[n++]=*ending++;}out[n]=0;return true;
}
void nameResult(uint8_t* destination,const char* text) {
    if(!destination)return;volatile uint8_t* out=destination;unsigned n=0;
    while(text[n]) {out[n+1]=text[n];++n;}out[0]=n;
}
bool macDate(const struct DateStamp& stamp,uint32_t& result) {
    if(stamp.ds_Days<0 || stamp.ds_Minute<0 || stamp.ds_Minute>=1440 || stamp.ds_Tick<0 || stamp.ds_Tick>=60*TICKS_PER_SECOND)return false;
    unsigned long long value=2335305600ULL+(unsigned long long)stamp.ds_Days*86400+(uint32_t)stamp.ds_Minute*60+(uint32_t)stamp.ds_Tick/TICKS_PER_SECOND;
    if(value>0xffffffffULL)return false;result=value;return true;
}
// Adapted from the pinned Alone in the Dark VolumeBacking service.
int32_t queryBacking(void* context) {
    MacVolumeBacking result={};int32_t error=0;BPTR lock=Lock((CONST_STRPTR)"PROGDIR:",ACCESS_READ);
    if(!lock)return -36;
    InfoData* info=(InfoData*)AllocMem(sizeof(InfoData),MEMF_PUBLIC|MEMF_CLEAR);
    FileInfoBlock* folder=(FileInfoBlock*)AllocDosObject(DOS_FIB,0);
    if(!info || !folder)error=-108;
    else if(!Info(lock,info) || !Examine(lock,folder))error=-36;
    else if(info->id_NumBlocks<=0 || info->id_NumBlocksUsed<0 || info->id_NumBlocksUsed>info->id_NumBlocks || info->id_BytesPerBlock<=0 ||
        (info->id_DiskState!=ID_VALIDATED && info->id_DiskState!=ID_WRITE_PROTECTED) || !info->id_VolumeNode || !macDate(folder->fib_Date,result.modified))error=MacFiles::unsupported;
    else {
        DosList* list=LockDosList(LDF_VOLUMES|LDF_READ);
        if(!list)error=-36;
        else {
            const DosList* volume=(const DosList*)BADDR(info->id_VolumeNode);
            if(volume->dol_Type!=DLT_VOLUME || !macDate(volume->dol_misc.dol_volume.dol_VolumeDate,result.created))error=MacFiles::unsupported;
            UnLockDosList(LDF_VOLUMES|LDF_READ);
        }
        result.blocks=info->id_NumBlocks;result.used=info->id_NumBlocksUsed;result.blockBytes=info->id_BytesPerBlock;result.locked=info->id_DiskState==ID_WRITE_PROTECTED;
    }
    if(folder)FreeDosObject(DOS_FIB,folder);if(info)FreeMem(info,sizeof(InfoData));UnLock(lock);
    if(!error) {
        *(MacVolumeBacking*)context=result;
#ifdef POP2_VERIFY
        if(g_fileVolumeQueries<128) {
            volatile uint32_t* row=g_fileVolume[g_fileVolumeQueries++];
            row[0]=result.blocks;row[1]=result.used;row[2]=result.blockBytes;
            row[3]=result.created;row[4]=result.modified;row[5]=result.locked;
        } else g_fileVolumeQueries=129;
#endif
    }
    return error;
}
struct DirectoryDate { const char* path;uint32_t date; };
int32_t queryDirectoryDate(void* context) {
    auto& query=*(DirectoryDate*)context;
    BPTR lock=Lock((CONST_STRPTR)(*query.path ? query.path : "PROGDIR:"),ACCESS_READ);if(!lock)return -36;
    FileInfoBlock* info=(FileInfoBlock*)AllocDosObject(DOS_FIB,0);int32_t error=0;
    if(!info)error=-108;
    else if(!Examine(lock,info) || info->fib_DirEntryType<0)error=-36;
    else if(!macDate(info->fib_Date,query.date))error=MacFiles::unsupported;
    if(info)FreeDosObject(DOS_FIB,info);UnLock(lock);return error;
}
bool sameName(const char* a,const char* b) { while(*a && *a==*b) {++a;++b;}return *a==*b; }
uint32_t metadataTime() {
    struct DateStamp stamp;DateStamp(&stamp);uint32_t date=0;
    return macDate(stamp,date) ? date : 0;
}
bool sizeOf(const char* path,uint32_t& size) {
    BPTR f=Open((CONST_STRPTR)path,MODE_OLDFILE);if(!f)return false;
    LONG end=Seek(f,0,OFFSET_END),length=end<0 ? -1 : Seek(f,0,OFFSET_BEGINNING);
    bool ok=Close(f) && length>=0;if(ok)size=length;return ok;
}
bool metadataRead(const char* path,FileMetadata::Record& record) {
    uint8_t raw[32],extra;BPTR f=Open((CONST_STRPTR)path,MODE_OLDFILE);if(!f)return false;
    bool ok=Read(f,raw,32)==32 && Read(f,&extra,1)==0;if(!Close(f))ok=false;
    return ok && FileMetadata::decode(raw,32,record);
}
bool discoverDirectory(uint32_t directory,const char* path) {
    BPTR lock=Lock((CONST_STRPTR)path,ACCESS_READ);if(!lock)return false;
    FileInfoBlock info __attribute__((aligned(4)));bool ok=Examine(lock,&info);LONG error=0;
    while(ok && ExNext(lock,&info)) {
        if(info.fib_DirEntryType>=0)continue;
        const char* full=(const char*)info.fib_FileName;unsigned n=0;while(full[n])++n;
        if(n<=6 || !sameName(full+n-6,".finfo"))continue;
        if(n-6>31) {ok=false;break;}
        char name[32];for(unsigned i=0;i<n-6;++i)name[i]=full[i];name[n-6]=0;
        if(files.child(directory,name))continue;
        MacFiles::Entry entry;error=files.planCreate(0,directory,name,entry);if(error) {ok=false;break;}
        char path[192];FileMetadata::Record record;uint32_t dataSize=0,resourceSize=0;
        ok=suffix(path,entry.path,".finfo") && metadataRead(path,record) && sizeOf(entry.path,dataSize) &&
            suffix(path,entry.path,".rsrc") && sizeOf(path,resourceSize);
        if(!ok)break;
        int32_t id=files.add(entry.parent,entry.name,entry.path,false,dataSize,resourceSize);
        if(id<0 || files.setMetadata(id,record)) {ok=false;break;}
    }
    if(ok && IoErr()!=ERROR_NO_MORE_ENTRIES)ok=false;
    UnLock(lock);return ok;
}
bool verify(const char* path,uint32_t size,const uint8_t expected[32],uint8_t* body=0) {
    BPTR f=Open((CONST_STRPTR)path,MODE_OLDFILE);if(!f)return false;
    bool ok=Seek(f,0,OFFSET_END)>=0 && Seek(f,0,OFFSET_BEGINNING)==(LONG)size;
#ifdef POP2_VERIFY
    SHA256 sha;sha_init(&sha);uint32_t done=0;
    while(ok && done<size) {
        uint32_t n=size-done;if(n>sizeof(scratch))n=sizeof(scratch);
        uint8_t* out=body ? body+done : scratch;
        ok=Read(f,out,n)==(LONG)n;if(ok)sha_update(&sha,out,n);done+=n;
    }
    if(!Close(f))ok=false;uint8_t digest[32];sha_final(&sha,digest);
    ok=ok && equal(digest,expected,32);
    if(ok) {++g_fileVerifiedFiles;g_fileVerifiedBytes+=size;}
#else
    // Release reads only requested startup metadata; installation has verified
    // hashes. There is no discard-only scan and no duplicate first-use hashing.
    (void)expected;
    if(ok && body)ok=Read(f,body,size)==(LONG)size;
    if(!Close(f))ok=false;
#endif
    return ok;
}
struct Verification { uint32_t id;bool resource; };
#ifdef POP2_VERIFY
int32_t verifyOperation(void* context) {
    Verification& v=*(Verification*)context;
    for(unsigned i=0;i<MacFiles::maxEntries;++i)if(sources[i].id==v.id) {
        Source& s=sources[i];unsigned fork=v.resource ? 1 : 0;
        if(s.verified[fork])return 0;
        char path[192];int32_t error=files.forkPath(v.id,v.resource,path,sizeof(path));if(error)return error;
        if(!verify(path,s.size[fork],s.hash[fork])) {
            // The options data fork is persistent user state. Its immutable
            // resource fork still uses the original hash. Installation verifies
            // the initial data; later runs accept the game's 16-byte settings.
            uint32_t size=0;
            if(!s.settings || v.resource || !sizeOf(path,size) || size!=s.size[0])return MacFiles::unsupported;
        }
        s.verified[fork]=true;return 0;
    }
    // Newly created files belong to this running catalog and have no original
    // input digest. Their metadata and lengths are set only after DOS succeeds.
    return files.entry(v.id) ? 0 : MacFiles::fnfErr;
}

#endif
int32_t verifyFork(uint32_t id,bool resource) {
#ifdef POP2_VERIFY
    Verification v={id,resource};return pop2SystemWindow(verifyOperation,&v);
#else
    (void)resource;return files.entry(id) ? 0 : MacFiles::fnfErr;
#endif
}
int32_t storeRecord(uint32_t id,const FileMetadata::Record& record) {
    const auto* entry=files.entry(id);char path[192];
    if(!entry || entry->directory || !suffix(path,entry->path,".finfo"))return -43;
    uint8_t bytes[32];FileMetadata::encode(record,bytes);
    BPTR f=Open((CONST_STRPTR)path,MODE_OLDFILE);if(!f)return -36;
    bool ok=Write(f,bytes,32)==32;if(!Close(f))ok=false;
    return ok ? files.setMetadata(id,record) : -36;
}
struct Request { uint32_t id,offset,count,actual;bool resource,writing;uint8_t* bytes; };
int32_t transfer(void* context) {
    Request& r=*(Request*)context;char path[192];
#ifdef POP2_FILE_PROBE
    Delay(2); // Positive scheduling/VBI control inside the DOS window.
#endif
#ifdef POP2_VERIFY
    Verification verification={r.id,r.resource};int32_t verified=verifyOperation(&verification);if(verified)return verified;
#endif
    int32_t error=files.forkPath(r.id,r.resource,path,sizeof(path));if(error)return error;
    BPTR f=Open((CONST_STRPTR)path,MODE_OLDFILE);
    if(!f)return IoErr()==ERROR_OBJECT_NOT_FOUND ? -43 : -36;
    if(Seek(f,r.offset,OFFSET_BEGINNING)<0)error=-36;
    while(!error && r.actual<r.count) {
        uint32_t n=r.count-r.actual;if(n>65536)n=65536;
        LONG actual=r.writing ? Write(f,r.bytes+r.actual,n) : Read(f,r.bytes+r.actual,n);
        if(actual<0)error=-36;
        else { r.actual+=actual;if(actual!=(LONG)n)break; }
    }
    if(!Close(f))error=-36;
    if(!error && r.writing && r.actual) {
        FileMetadata::Record record=files.entry(r.id)->metadata;record.modified=metadataTime();
        if(!record.modified)return MacFiles::unsupported;
        error=storeRecord(r.id,record);
    }
    return error;
}
int32_t read(void*,uint32_t id,bool resource,uint32_t at,uint8_t* bytes,uint32_t count,uint32_t& actual) {
    actual=0;if(at>0x7fffffffUL || count>0x7fffffffUL-at)return -50;
    Request r={id,at,count,0,resource,false,bytes};int32_t error=pop2SystemWindow(transfer,&r);actual=r.actual;return error;
}
int32_t write(void*,uint32_t id,bool resource,uint32_t at,const uint8_t* bytes,uint32_t count,uint32_t& actual) {
    actual=0;if(at>0x7fffffffUL || count>0x7fffffffUL-at)return -50;
    // D9 leaves installed assets immutable; settings and newly created saves
    // are the writable data. Opening an asset with fsRdWrPerm is still allowed,
    // as the original audio archive reader requests that permission.
    for(const auto& source:sources)if(source.id==id && (resource || !source.settings))return MacFiles::unsupported;
    Request r={id,at,count,0,resource,true,(uint8_t*)bytes};int32_t error=pop2SystemWindow(transfer,&r);actual=r.actual;return error;
}
const FileTransfers::Backend operations={0,read,write};
struct Create { MacFiles::Entry entry;FileMetadata::Record record;uint32_t id; };
int32_t createOperation(void* context) {
    Create& c=*(Create*)context;char paths[3][192];
    if(!suffix(paths[0],c.entry.path,"") || !suffix(paths[1],c.entry.path,".rsrc") || !suffix(paths[2],c.entry.path,".finfo"))return -50;
    for(unsigned i=0;i<3;++i) {
        BPTR lock=Lock((CONST_STRPTR)paths[i],ACCESS_READ);
        if(lock) {UnLock(lock);return -48;}
        if(IoErr()!=ERROR_OBJECT_NOT_FOUND)return -36;
    }
    c.record.created=c.record.modified=metadataTime();if(!c.record.created)return MacFiles::unsupported;
    uint8_t metadata[32];FileMetadata::encode(c.record,metadata);
    for(unsigned i=0;i<3;++i) {
        BPTR f=Open((CONST_STRPTR)paths[i],MODE_NEWFILE);if(!f)return -36;
        bool ok=i!=2 || Write(f,metadata,32)==32;
        if(!Close(f))ok=false;if(!ok)return -36;
    }
    int32_t added=files.add(c.entry.parent,c.entry.name,c.entry.path,false);
    if(added<0)return added;c.id=added;return files.setMetadata(c.id,c.record);
}
struct Metadata { uint32_t id;FileMetadata::Record record; };
int32_t metadataOperation(void* context) {
    Metadata& m=*(Metadata*)context;return storeRecord(m.id,m.record);
}
}
namespace NativeFiles {
MacFiles& catalog() { return files; }
const FileTransfers::Backend& backend() { return operations; }
bool prepare() {
    BPTR f=Open((CONST_STRPTR)"file-catalog.bin",MODE_OLDFILE);if(!f)return false;
    LONG old=Seek(f,0,OFFSET_END),length=old<0 ? -1 : Seek(f,0,OFFSET_BEGINNING);Close(f);
    if(length<8 || length>(LONG)sizeof(catalogBytes) || !verify("file-catalog.bin",length,fileCatalogSHA256,catalogBytes))return false;
    if(!equal(catalogBytes,(const uint8_t*)"PFC1",4) || lng(catalogBytes+4)!=32)return false;
    for(unsigned i=0;i<MacFiles::maxEntries;++i)sources[i]={};
    files.reset();files.application=files.add(2,"Prince of Persia 2","PROGDIR:",true);
    files.data=files.add(files.application,"Data","Data",true);files.applicationComplete=true;
    uint32_t at=8;
    for(unsigned i=0;i<32;++i) {
        if(at+2>(uint32_t)length)return false;unsigned n=word(catalogBytes+at);at+=2;
        if(!n || n>=160 || at+n+104>(uint32_t)length)return false;
        char name[160];for(unsigned j=0;j<n;++j)name[j]=catalogBytes[at+j];name[n]=0;at+=n;
        uint32_t ds=lng(catalogBytes+at),rs=lng(catalogBytes+at+36);
        char path[192];
        bool settings=sameName(name,"Data/Prince2.opt");
        uint8_t metadata[32];FileMetadata::Record record;
        if(!suffix(path,name,".finfo"))return false;
        if(verify(path,32,catalogBytes+at+72,metadata)) {
            if(!FileMetadata::decode(metadata,32,record))return false;
            // The verified Macintosh installation clears Finder's initialized
            // bit (install_reference_volume.py). Expose that installed state,
            // while retaining the original AFI1 bytes and their input hash.
            record.finder[8]&=0xfe;
        } else if(!settings || !metadataRead(path,record))return false;
        bool data=n>5 && name[0]=='D' && name[1]=='a' && name[2]=='t' && name[3]=='a' && name[4]=='/';
        int32_t id=files.add(data ? files.data : files.application,data ? name+5 : name,name,false,ds,rs);
        if(id<0 || files.setMetadata(id,record))return false;
        Source& source=sources[i];source.settings=settings;source.id=id;source.size[0]=ds;source.size[1]=rs;
        for(unsigned k=0;k<32;++k) {source.hash[0][k]=catalogBytes[at+4+k];source.hash[1][k]=catalogBytes[at+40+k];}
        at+=104;
    }
    if(at!=(uint32_t)length || !discoverDirectory(files.data,"Data") || !discoverDirectory(files.application,"PROGDIR:"))return false;
#ifdef POP2_VERIFY
    // Verify immutable inputs before original startup can start music or timers.
    // Reuse the same verifier and mutable-options rule as first-use opens.
    for(auto& source:sources)if(source.id)for(unsigned fork=0;fork<2;++fork){
        Verification request={source.id,fork!=0};
        if(verifyOperation(&request))return false;
    }
#endif
    return true;
}
int32_t openFork(int16_t volume,uint32_t directory,const char* name,bool resource,uint8_t permission,int16_t& ref) {
    uint32_t id=0;ref=0;int32_t error=files.resolve(volume,directory,name,id,true);
    if(!error)error=verifyFork(id,resource);
    if(!error)error=files.openFork(id,resource,permission,false,ref);
    return error;
}
bool handles(uint16_t trap,uint16_t selector) {
    return trap==0xa000 || trap==0xa200 || trap==0xa00a || trap==0xa20a || trap==0xa001 ||
        trap==0xa002 || trap==0xa003 || trap==0xa011 || trap==0xa044 || trap==0xa013 ||
        trap==0xa008 || trap==0xa207 || trap==0xa00c || trap==0xa00d || trap==0xa014 || trap==0xa214 || trap==0xa015 || ((trap==0xa060 || trap==0xa260) && (selector==26 || selector==8 || selector==9));
}
int32_t dispatch(uint16_t trap,uint16_t selector,uint8_t* pb,uint8_t* buffer,const char* name,uint8_t* nameOutput) {
    if((trap==0xa060 || trap==0xa260) && selector==26)trap=(trap&0x200) ? 0xa200 : 0xa000;
    int16_t ref=(int16_t)word(pb+24),volume=(int16_t)word(pb+22);
    if(trap==0xa207) {
        uint32_t directory=0;int16_t selected=volume;
        int32_t error=files.selectVolume(selected,(int16_t)word(pb+28),name,directory);
        if(!error) {
            MacVolumeBacking backing={};error=pop2SystemWindow(queryBacking,&backing);
            if(!error)error=files.volumeInfo(directory,backing,pb);
            if(!error)nameResult(nameOutput,files.entry(2)->name);
        }
        putWord(pb+22,selected);return error;
    }
    bool catalog=(trap==0xa060 || trap==0xa260) && selector==9;
    uint32_t catalogID=0;
    if(catalog) {
        int16_t index=(int16_t)word(pb+28);if(index>0)return MacFiles::unsupported;
        int32_t error=files.resolve(volume,lng(pb+48),index<0 ? 0 : name,catalogID);
        if(error)return error;const MacFiles::Entry* entry=files.entry(catalogID);
        if(entry->directory) {
            DirectoryDate dates={entry->path,0};error=pop2SystemWindow(queryDirectoryDate,&dates);if(error)return error;
            uint16_t count=0;
            for(uint32_t id=2;id<MacFiles::maxEntries+2;++id) {const auto* child=files.entry(id);if(child && child->parent==catalogID)++count;}
            pb[30]=0x10;pb[31]=0;
            for(unsigned i=32;i<48;++i)pb[i]=0; // Port directories have no Finder window state (D8).
            putLong(pb+48,catalogID);putWord(pb+52,count);
            // DOS has one directory timestamp; both Mac date fields describe
            // that backing directory. Reserved bytes remain untouched.
            putLong(pb+72,dates.date);putLong(pb+76,dates.date);putLong(pb+80,0);
            for(unsigned i=84;i<100;++i)pb[i]=0;
            putLong(pb+100,entry->parent);nameResult(nameOutput,entry->name);return 0;
        }
        if(index<0)return MacFiles::dirNFErr;
    }
    if(trap==0xa014 || trap==0xa214) {
        uint32_t directory=0;int32_t error=files.directoryFor(0,directory);if(error)return error;
        nameResult(nameOutput,files.entry(2)->name);putWord(pb+22,files.defaultRef());
        if(trap==0xa214) {putWord(pb+32,MacFiles::volumeRef);putLong(pb+48,directory);}return 0;
    }
    if(trap==0xa015)return files.setDefault(volume,name);
    if(trap==0xa00c || trap==0xa00d || catalog) {
        uint32_t id=catalogID;int16_t index=(int16_t)word(pb+28);
        int32_t error=catalog ? 0 : trap==0xa00c && index>0 ? files.indexedFile(volume,0,index,id) : files.resolve(volume,0,name,id);
        if(error)return error;const MacFiles::Entry* entry=files.entry(id);
        if(entry->directory || !entry->metadataKnown)return MacFiles::unsupported;
        if(trap==0xa00d) {
            FileMetadata::Record record;
            for(unsigned i=0;i<16;++i)record.finder[i]=pb[32+i];
            record.created=lng(pb+72);record.modified=lng(pb+76);return metadata(id,record);
        }
        uint8_t attributes=0;int16_t first=0;
        for(int16_t i=1;i<=MacFiles::maxOpen;++i) {
            const MacFiles::Fork* fork=0;if(files.queryFork(0,i,0,fork))break;
            if(fork->id==id) {if(!first)first=fork->ref;attributes|=0x80|(fork->resource ? 4 : 8);}
        }
        putWord(pb+24,first);pb[30]=attributes;pb[31]=0;
        for(unsigned i=0;i<16;i+=4)putLong(pb+32+i,lng(entry->metadata.finder+i));
        putLong(pb+48,id);putWord(pb+52,0);putLong(pb+54,entry->dataSize);putLong(pb+58,entry->dataSize);
        putWord(pb+62,0);putLong(pb+64,entry->resourceSize);putLong(pb+68,entry->resourceSize);
        putLong(pb+72,entry->metadata.created);putLong(pb+76,entry->metadata.modified);
        if(catalog) {for(unsigned i=80;i<100;++i)pb[i]=0;putLong(pb+100,entry->parent);putLong(pb+104,0);}
        // Named GetFInfo preserves the caller's path; only indexed queries
        // return a leaf name. Measured by the original Hall/save Create calls.
        if(catalog || index>0)nameResult(nameOutput,entry->name);return 0;
    }
    if((trap==0xa060 || trap==0xa260) && selector==8) {
        const MacFiles::Fork* fork=0;
        int32_t error=files.queryFork(volume,(int16_t)word(pb+28),ref,fork);
        if(error)return error;
        const MacFiles::Entry* entry=files.entry(fork->id);
        if(nameOutput) {
            volatile uint8_t* output=nameOutput;unsigned n=0;
            while(entry->name[n]) {output[n+1]=entry->name[n];++n;}output[0]=n;
        }
        uint32_t size=fork->resource ? entry->resourceSize : entry->dataSize;
        putWord(pb+24,fork->ref);putLong(pb+32,entry->id);
        putWord(pb+36,(fork->resource ? 0x200 : 0)|(fork->writable ? 0x100 : 0)|
            (fork->shared ? 0x1000 : 0)|(fork->locked ? 0x2000 : 0)|(fork->modified ? 0x8000 : 0));
        putWord(pb+38,0);putLong(pb+40,size);putLong(pb+44,size);
        putLong(pb+48,fork->position);putWord(pb+52,MacFiles::volumeRef);
        putLong(pb+54,0);putLong(pb+58,entry->parent);return 0;
    }
    if(trap==0xa000 || trap==0xa200 || trap==0xa00a || trap==0xa20a) {
        if(!name)return MacFiles::paramErr;
        int16_t opened=0;
        int32_t error=openFork(volume,(trap&0x200) ? lng(pb+48) : 0,name,(trap&0xff)==0x0a,pb[27],opened);
        putWord(pb+24,opened);return error;
    }
    if(trap==0xa001)return files.close(ref);
    if(trap==0xa011) { uint32_t size=0;int32_t error=FileTransfers::eof(files,ref,size);if(!error)putLong(pb+28,size);return error; }
    if(trap==0xa044) {
        int32_t error=files.seek(ref,word(pb+44),(int32_t)lng(pb+46));
        const MacFiles::Fork* fork=files.fork(ref);if(fork)putLong(pb+46,fork->position);return error;
    }
    if(trap==0xa002 || trap==0xa003) {
        FileTransfers::Result result=trap==0xa002
            ? FileTransfers::read(files,operations,ref,word(pb+44),(int32_t)lng(pb+46),buffer,lng(pb+36))
            : FileTransfers::write(files,operations,ref,word(pb+44),(int32_t)lng(pb+46),buffer,lng(pb+36));
        putLong(pb+40,result.actual);if(files.fork(ref))putLong(pb+46,result.position);return result.error;
    }
    if(trap==0xa008) {if(pb[26])return MacFiles::unsupported;uint32_t id=0;return create(volume,0,name,0,0,id);}
    if(trap==0xa013)return files.volume(volume,name); // Every transfer closes and flushes its DOS handle.
    return MacFiles::unsupported;
}
int32_t create(int16_t volume,uint32_t directory,const char* name,uint32_t type,uint32_t creator,uint32_t& id) {
    Create c={};int32_t error=files.planCreate(volume,directory,name,c.entry);if(error)return error;
    for(unsigned i=0;i<4;++i) { c.record.finder[i]=type>>(24-i*8);c.record.finder[4+i]=creator>>(24-i*8); }
    error=pop2SystemWindow(createOperation,&c);if(!error)id=c.id;return error;
}
int32_t makeSpec(int16_t volume,uint32_t directory,const char* path,uint8_t* spec) {
    uint32_t id=0,parent=0;const char* leaf=0;char prefix[256];
    int32_t error=files.resolve(volume,directory,path,id);
    if(!error) {const auto* entry=files.entry(id);parent=entry->parent;leaf=entry->name;}
    else if(error==MacFiles::fnfErr) {
        const char* last=0;for(const char* p=path;*p;++p)if(*p==':')last=p;
        if(last) {
            unsigned n=last-path+1;if(n>=sizeof(prefix))return -50;
            for(unsigned i=0;i<n;++i)prefix[i]=path[i];prefix[n]=0;
            int32_t parentError=files.resolve(volume,directory,prefix,parent);if(parentError)return parentError;
            leaf=last+1;
        } else {int32_t parentError=files.resolve(volume,directory,0,parent);if(parentError)return parentError;leaf=path;}
        if(!files.entry(parent)->directory)return MacFiles::dirNFErr;
    } else return error;
    unsigned n=0;while(leaf[n])++n;if(!n || n>31)return -37;
    putWord(spec,MacFiles::volumeRef);putLong(spec+2,parent);nameResult(spec+6,leaf);return error;
}
int32_t metadata(uint32_t id,const FileMetadata::Record& record) {
    Metadata m={id,record};return pop2SystemWindow(metadataOperation,&m);
}
}
#endif
