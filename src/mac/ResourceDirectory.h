#ifndef AMIGAXDEV_RESOURCE_DIRECTORY_H
#define AMIGAXDEV_RESOURCE_DIRECTORY_H
#include "ResourceWriter.h"
// Mutable metadata only. Caller-owned payload sources must remain readable until
// replaced, rebased or closed; the directory never owns file handles or bodies.
class ResourceDirectory {
public:
    static const uint16_t maximumForks=16,maximumResources=ResourceForks::kMaximumResources;
    using Entry=ResourceWriter::Entry;
    struct View { uint32_t identity;int16_t ref;Entry entry; };
    ResourceDirectory()=default;
    ~ResourceDirectory() { clear(); }
    ResourceDirectory(const ResourceDirectory&)=delete;
    ResourceDirectory& operator=(const ResourceDirectory&)=delete;
    int32_t open(int16_t ref,const ResourceForks::Source& source,bool writable);
    int32_t create(int16_t ref); // Empty writable map, not a disk operation.
    int32_t close(int16_t ref);
    void clear();
    bool active(int16_t ref) const;
    bool dirty(int16_t ref) const;
    bool newest(int16_t& ref) const;
    bool older(int16_t ref,int16_t& next) const;
    uint16_t count(int16_t ref) const;
    bool at(int16_t ref,uint16_t ordinal,View& out) const; // zero-based, insertion order
    bool get(uint32_t identity,View& out) const;
    bool find(int16_t ref,uint32_t type,int16_t id,View& out) const;
    int32_t add(int16_t ref,const Entry& entry,uint32_t& identity);
    int32_t replace(uint32_t identity,const Entry& entry);
    int32_t remove(uint32_t identity); // In-memory removal also works on read-only maps.
    int32_t read(uint32_t identity,uint32_t offset,uint8_t* out,uint32_t size) const;
    // Select a publication body without changing live metadata or saved sources.
    // The caller keeps this selection and its sources stable through rebase.
    // Unselected entries retain their existing source; no body is preloaded.
    // Returning omitEntry excludes a record from disk but retains its live identity,
    // name and original source. That source must survive publication independently.
    static const int32_t omitEntry=1;
    using PayloadOverride=int32_t (*)(void*,uint32_t identity,ResourceForks::Source&,uint32_t& offset,uint32_t& size);
    // Successful serialization does not release old sources or clear dirty.
    int32_t serialize(int16_t ref,const ResourceWriter::Sink& sink,PayloadOverride select=0,void* context=0) const;
    // Validate a just-published map against current metadata, then atomically
    // rebind included sources/offsets while retaining resource identities. Omitted
    // entries keep the map dirty until a later complete publication.
    int32_t rebase(int16_t ref,const ResourceForks::Source& source,PayloadOverride select=0,void* context=0);
private:
    struct Fork { bool active=false,writable=false,dirty=false;int16_t ref=0;uint32_t opened=0;uint8_t* map=0; };
    struct Record { uint32_t identity=0;uint16_t fork=0;Entry entry={};uint8_t* name=0; };
    Fork forks_[maximumForks];Record records_[maximumResources];uint32_t nextIdentity_=1;
#ifdef AMIGAXDEV_RESOURCE_DIRECTORY_INDEXED
    // Vette's sorted lookup, with identities breaking ties for duplicate keys.
    mutable uint16_t lookup_[maximumResources],lookupCount_=0;
    mutable bool lookupValid_=false;
    bool lookupBefore(uint16_t a,uint16_t b) const;
    void prepareLookup() const;
#endif
    uint16_t order(int16_t fork,uint16_t* indices) const;
    int16_t forkIndex(int16_t ref) const;
    int16_t recordIndex(uint32_t identity) const;
    int32_t mutation(int16_t fork,int16_t replaced,const Entry* entry) const;
    static int32_t parse(const ResourceForks::Source&,uint8_t*& map,Entry*& entries,uint16_t& count);
    int32_t publication(int16_t fork,PayloadOverride,void*,Entry*& recipe,uint16_t* indices,uint16_t& count) const;
    static uint8_t* copyName(const Entry& entry);
};
#endif
