#pragma once
#ifndef POP2_PLATFORM_AMIGA
#include <cstdint>
#endif
#include "ResourceMap.h"

// Borrowed, immutable, hash-verified application resource fork. The caller keeps
// the input storage alive. All addresses here are file offsets, never Mac PCs.
class OriginalApplication {
public:
    struct Resource { const uint8_t* bytes; uint32_t size; uint8_t attributes; };
    struct World { uint32_t above,below,jumpBytes,jumpOffset; };
    bool open(const uint8_t* bytes,uint32_t size,const uint8_t expectedSHA256[32]);
    void close();
    bool resource(uint32_t type,int16_t id,Resource& out) const;
    const World& world() const { return world_; }
    const char* error() const { return error_; }
private:
    const uint8_t* bytes_=0;
    ResourceMap map_;
    World world_={};
    const char* error_="APPLICATION / NOT OPEN";
    bool fail(const char* error);
    bool startupLayout();
};
