#ifndef POP2_FILE_READ_CACHE_H
#define POP2_FILE_READ_CACHE_H
// Caller owns the 64 KiB buffer and open-fork context. No allocation or OS calls.
class FileReadCache {
public:
    static const uint32_t capacity=65536;
    typedef int32_t (*Reader)(void*,uint32_t,uint8_t*,uint32_t,uint32_t&);
    void bind(uint8_t* buffer,uint32_t size,Reader reader,void* context);
    int32_t read(uint32_t offset,uint8_t* out,uint32_t bytes,uint32_t& actual);
private:
    uint8_t* buffer_=0;
    uint32_t size_=0,start_=0,valid_=0;
    Reader reader_=0;
    void* context_=0;
};
#endif
