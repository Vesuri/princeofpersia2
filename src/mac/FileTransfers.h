#ifndef POP2_FILE_TRANSFERS_H
#define POP2_FILE_TRANSFERS_H
#include "MacFiles.h"
// Synchronous fork transfers. The backend owns storage and any OS handback;
// the catalog owns Macintosh reference numbers, permissions and file marks.
namespace FileTransfers {
struct Backend {
    void* context;
    int32_t (*read)(void*,uint32_t id,bool resource,uint32_t offset,uint8_t*,uint32_t,uint32_t&);
    int32_t (*write)(void*,uint32_t id,bool resource,uint32_t offset,const uint8_t*,uint32_t,uint32_t&);
};
struct Result { int32_t error;uint32_t actual,position; };
Result read(MacFiles&,const Backend&,int16_t ref,uint16_t mode,int32_t offset,uint8_t*,uint32_t count);
Result write(MacFiles&,const Backend&,int16_t ref,uint16_t mode,int32_t offset,const uint8_t*,uint32_t count);
int32_t eof(const MacFiles&,int16_t ref,uint32_t& size);
}
#endif
