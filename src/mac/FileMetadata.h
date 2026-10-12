#ifndef POP2_FILE_METADATA_H
#define POP2_FILE_METADATA_H
// Port-owned Finder metadata companion. Resource bytes use a separate .rsrc.
// AFI1, 16 Finder bytes, two big-endian Mac dates, FNV-1a of the first 28 bytes.
namespace FileMetadata {
struct Record { uint8_t finder[16]; uint32_t created,modified; };
const uint32_t bytes=32;
void encode(const Record& record,uint8_t* out);
bool decode(const uint8_t* input,uint32_t length,Record& record);
}
#endif
