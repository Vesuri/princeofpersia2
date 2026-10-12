#ifndef POP2_RESOURCE_WRITER_H
#define POP2_RESOURCE_WRITER_H
#include "ResourceForks.h"
// Bounded serializer for an immutable resource recipe. The caller owns names
// and sources for the entire transaction; unchanged payloads stay source-backed.
class ResourceWriter {
public:
    struct Entry {
        uint32_t type;int16_t id;uint8_t attrs;
        const uint8_t* name;uint8_t nameLength; // null means unnamed, nonnull may be empty.
        ResourceForks::Source source;uint32_t offset,size;
    };
    struct Sink {
        void* context;
        // Must create isolated staging storage, never truncate an existing fork.
        int32_t (*begin)(void*,uint32_t size);
        int32_t (*write)(void*,uint32_t offset,const uint8_t*,uint32_t size,uint32_t& actual);
        // publish=true atomically replaces the target only on success; false
        // discards staging. A failed publication must leave the old target intact.
        int32_t (*finish)(void*,bool publish);
    };
    // Types follow first appearance; reference lists retain recipe order within
    // each type, including duplicate type/ID pairs (Mac reference M2.2f4a1).
    // Validate without reading payloads or allocating staging storage.
    static int32_t measure(const Entry* entries,uint16_t count,uint32_t& size);
    static int32_t serialize(const Entry* entries,uint16_t count,const Sink& sink);
};
#endif
