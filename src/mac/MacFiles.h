#ifndef POP2_MAC_FILES_H
#define POP2_MAC_FILES_H
#include "FileMetadata.h"
#include "MacVolume.h"
// Portable metadata/open-fork model. File payloads and host allocation are external.
class MacFiles {
public:
    enum { maxEntries=128, maxOpen=16, maxWD=16, noErr=0, fnfErr=-43,
        paramErr=-50, rfNumErr=-51, nsvErr=-35, dirNFErr=-120, unsupported=-32760 };
    enum { fcbLength=94, fcbTableSize=2+maxOpen*fcbLength };
    void writeFCBTable(uint8_t* table,uint32_t volumeControlBlock);
    bool fcbTableDirty() const { return fcbTableDirty_; }
    struct Entry {
        uint32_t id,parent,dataSize,resourceSize;
        bool directory,metadataKnown,metadataDirty,resourceIsBase;
        FileMetadata::Record metadata;
        char name[32];
        char path[160];
    };
    struct Fork { int16_t ref; uint32_t id,position; bool resource,writable,shared,locked,modified; };
    void reset();
    int32_t add(uint32_t parent,const char* name,const char* path,bool directory,
                uint32_t dataSize=0,uint32_t resourceSize=0,bool resourceIsBase=false);
    int16_t forkPath(uint32_t id,bool resource,char* path,uint32_t capacity) const;
    const Entry* entry(uint32_t id) const;
    const Entry* child(uint32_t parent,const char* name) const;
    int16_t resolve(int16_t volume,uint32_t directory,const char* path,uint32_t& id,bool fileOpen=false) const;
    int16_t indexedFile(int16_t volume,uint32_t directory,int16_t index,uint32_t& id) const;
    // Validate a creation without touching the catalog or disk. add commits it
    // only after the backend has created the complete native representation.
    int16_t planCreate(int16_t volume,uint32_t directory,const char* path,Entry& candidate) const;
    int16_t setMetadata(uint32_t id,const FileMetadata::Record& metadata,bool dirty=false);
    void touchMetadata(uint32_t id,uint32_t date);
    void metadataFlushed(uint32_t id);
    int16_t canRemove(uint32_t id) const;
    int16_t remove(uint32_t id);
    int16_t open(uint32_t id,bool resource,bool writable);
    int16_t openFork(uint32_t id,bool resource,uint8_t permission,bool locked,int16_t& ref);
    int16_t openData(uint32_t id,uint8_t permission,bool locked,int16_t& ref) { return openFork(id,false,permission,locked,ref); }
    int16_t volume(int16_t ref,const char* name) const;
    int16_t volumeParameters(int16_t ref,const char* name,uint8_t* buffer,uint32_t requested,uint32_t& actual) const;
    int16_t selectVolume(int16_t& ref,int16_t index,const char* name,uint32_t& directory) const;
    int16_t volumeInfo(uint32_t directory,const MacVolumeBacking& backing,uint8_t* parameterBlock) const;
    void modified(int16_t ref);
    void flushed(uint32_t id,bool resource=false);
    const Fork* fork(int16_t ref) const;
    int16_t queryFork(int16_t volume,int16_t index,int16_t ref,const Fork*& found) const;
    int16_t close(int16_t ref);
    int16_t seek(int16_t ref,uint16_t mode,int32_t offset,bool writing=false);
    int16_t setSize(int16_t ref,uint32_t size,bool clampPosition);
    void advance(int16_t ref,uint32_t count);
    int16_t initializeDirectories();
    int16_t queryWD(int16_t& ref,int16_t index,uint32_t& process,uint32_t& directory) const;
    int16_t openWD(uint32_t directory,uint32_t process,bool* created=0);
    int16_t closeWD(int16_t ref);
    int16_t directoryFor(int16_t ref,uint32_t& directory) const;
    uint32_t wdProcess(int16_t ref) const;
    int16_t setDefault(int16_t ref,const char* volumeName=0);
    int16_t setHierarchicalDefault(int16_t ref,uint32_t directory,const char* path=0);
    int16_t defaultRef() const { return defaultRef_ ? defaultRef_ : application ? applicationWD : volumeRef; }
    uint16_t count() const { return count_; }
    uint32_t application=0,system=0,preferences=0,saves=0,data=0;
    bool applicationComplete=false;
    static const int16_t volumeRef=-1,applicationWD=-32000,systemWD=(int16_t)0x8053;
private:
    bool fcbTableDirty_=true;
    struct WD { int16_t ref; uint32_t directory,process; };
    Entry entries_[maxEntries]; Fork forks_[maxOpen]; WD wd_[maxWD];
    uint16_t count_=0,used_=0;
    uint32_t nextID_=2;
    int16_t defaultRef_=0;
    uint32_t defaultDirectory_=0;
};
#endif
