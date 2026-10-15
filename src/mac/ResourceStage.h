#ifndef AMIGAXDEV_RESOURCE_STAGE_H
#define AMIGAXDEV_RESOURCE_STAGE_H
#include "ResourceWriter.h"
// Explicit-lifetime DOS staging sink. All callbacks require the active Mac user-service bridge and enter
// bounded system windows. Caller closes old target streams before publication.
// Existing temporary/backup files are recovery evidence, never overwritten.
class ResourceStage {
public:
    static const int32_t recoveryRequired=-32760;
    bool bind(const char* path);
    ResourceWriter::Sink sink() { return {this,begin,write,finish}; }
    bool active() const { return owned_ || handle_ || failed_; }
private:
    char path_[192]={},temporary_[192]={},backup_[192]={};
    uint8_t* resloadBytes_=0;
    uint32_t handle_=0,size_=0,written_=0;
    bool owned_=false,previous_=false,failed_=false,poisoned_=false;
    static int32_t begin(void*,uint32_t);
    static int32_t write(void*,uint32_t,const uint8_t*,uint32_t,uint32_t&);
    static int32_t finish(void*,bool);
    static int32_t beginInside(void*);
    static int32_t writeInside(void*);
    static int32_t finishInside(void*);
};
#endif
