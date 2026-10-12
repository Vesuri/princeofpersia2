#ifndef POP2_FILE_WRITE_BUFFER_H
#define POP2_FILE_WRITE_BUFFER_H
// Sparse dirty-page overlay. Backends/allocators own OS windows and storage.
class FileWriteBuffer {
public:
    FileWriteBuffer()=default;
    FileWriteBuffer(const FileWriteBuffer&)=delete;
    FileWriteBuffer& operator=(const FileWriteBuffer&)=delete;
    enum { pageBytes=65536, maxPages=32, unsupported=-32760 };
    typedef int32_t (*Reader)(void*,uint32_t,uint8_t*,uint32_t,uint32_t&);
    typedef int32_t (*Writer)(void*,uint32_t,const uint8_t*,uint32_t,uint32_t&);
    typedef int32_t (*Resizer)(void*,uint32_t);
    typedef uint8_t* (*Allocator)(uint32_t);
    typedef void (*Releaser)(uint8_t*,uint32_t);
    int32_t bind(uint32_t size,Reader reader,void* context,Allocator allocate,Releaser release);
    // Explicit disposal; caller must flush first when retaining modifications.
    void clear();
    int32_t read(uint32_t offset,uint8_t* bytes,uint32_t count,uint32_t& actual) const;
    int32_t write(uint32_t offset,const uint8_t* bytes,uint32_t count,uint32_t& actual);
    int32_t resize(uint32_t size);
    // Caller runs callbacks in its chosen bounded system window. On any failure
    // all dirty state remains available for a retry; success releases the pages.
    int32_t flush(Writer writer,Resizer resizer,void* context);
    // Backends without resize/rename can publish a fully assembled file once.
    typedef int32_t (*Replacer)(void*,const uint8_t*,uint32_t);
    int32_t flushWhole(Replacer replace,void* context);
    uint32_t size() const { return size_; }
    bool dirty() const { return dirty_; }
private:
    struct Page { uint32_t offset;uint8_t* bytes; } pages_[maxPages]={};
    uint32_t size_=0,sourceLimit_=0;
    Reader reader_=0;void* context_=0;Allocator allocate_=0;Releaser release_=0;
    bool dirty_=false;
    const Page* page(uint32_t offset) const;
    int32_t original(uint32_t offset,uint8_t* bytes,uint32_t count,uint32_t* completed=0) const;
};
#endif
