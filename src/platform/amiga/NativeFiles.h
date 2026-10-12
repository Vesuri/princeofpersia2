#pragma once
#include "mac/MacFiles.h"
#include "mac/FileTransfers.h"
namespace NativeFiles {
bool prepare();
bool handles(uint16_t trap,uint16_t selector);
int32_t dispatch(uint16_t trap,uint16_t selector,uint8_t* parameterBlock,uint8_t* buffer,const char* name,uint8_t* nameOutput=0);
int32_t openFork(int16_t volume,uint32_t directory,const char* name,bool resource,uint8_t permission,int16_t& ref);
MacFiles& catalog();
const FileTransfers::Backend& backend();
int32_t create(int16_t volume,uint32_t directory,const char* name,uint32_t type,uint32_t creator,uint32_t& id);
int32_t makeSpec(int16_t volume,uint32_t directory,const char* path,uint8_t* spec);
int32_t metadata(uint32_t id,const FileMetadata::Record& record);
}
