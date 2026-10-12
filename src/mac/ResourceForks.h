#ifndef POP2_RESOURCE_FORKS_H
#define POP2_RESOURCE_FORKS_H
// Resource directory and bounded payload access. Source contexts remain owned
// by the caller; the directory owns only map bytes and resource metadata.
class ResourceDirectory;
class ResourceForks {
public:
    ResourceForks()=default;
    ~ResourceForks() { close(); }
    ResourceForks(const ResourceForks&)=delete;
    ResourceForks& operator=(const ResourceForks&)=delete;
    static const uint16_t kForkCount=16,kMaximumResources=768;
    static const uint32_t chunkBytes=65536,maximumMapBytes=262144;
    static const uint16_t kOverlayFork=kForkCount-1;
    struct Source {
        void* context;
        uint32_t size;
        int32_t (*read)(void*,uint32_t,uint8_t*,uint32_t,uint32_t&);
    };
    struct Item {
        uint16_t fork;int16_t id;uint32_t type;uint8_t attrs;
        const uint8_t* name;uint8_t nameLength;
        const uint8_t* data; // Resident compatibility only; null for file sources.
        uint32_t size;
    };
    // Resident compatibility entry, removed from startup when platform I/O lands.
    bool open(const uint8_t* application,uint32_t applicationSize,const uint8_t* data,uint32_t dataSize);
    bool open(const Source& application,const Source* data=0);
    // Overlay is older than the application; later dynamic files precede both.
    // Both sources remain read-only and bodies are fetched only on demand.
    bool openWithOverlay(const Source& application,const Source& overlay);
    void close();
    // Native mutation uses the directory, then refreshes this dense view before
    // any indexed access. Internal fork keys must remain in [0,kForkCount).
    ResourceDirectory* directory() { return m_directory; }
    uint32_t identity(uint32_t index) const;
    // oldToNew has kMaximumResources entries. Removed identities map to -1;
    // surviving identities let callers move their handle associations safely.
    // A refreshed view is source-backed (resident compatibility pointers clear).
    bool refresh(int16_t* oldToNew);

    uint16_t forkCount() const { return m_open ? m_forks : 0; }
    uint32_t resourceCount() const { return m_count; }
    // Items retain each fork's original map reference-list order.
    bool item(uint32_t index,Item& out) const;
    // D5: dialog layouts override ordinary search; single-file calls stay local.
    // Caller provides kForkCount entries. No payload reads are performed.
    uint16_t searchOrder(uint16_t current,uint32_t type,bool currentOnly,uint16_t* keys) const;
    bool find(uint16_t fork,uint32_t type,int16_t id,Item& out,uint32_t* index=0) const;
    // Reads exactly the indexed resource into caller-owned storage, in <=64 KiB
    // transfers. Errors/short reads never claim a complete resource.
    int32_t read(uint32_t index,uint8_t* destination,uint32_t capacity) const;
private:
    bool appendFork(uint16_t fork,const Source& source,const uint8_t* resident=0);
    struct Record { Item item;uint32_t identity; };
    Record m_items[kMaximumResources];
    ResourceDirectory* m_directory=0;
    uint16_t m_count=0,m_forks=0;
    bool m_open=false;
    bool finish();
};
#endif
