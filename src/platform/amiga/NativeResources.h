#pragma once
#include "mac/ResourceDirectory.h"
namespace NativeResources {
struct Item { uint32_t identity,type,size;int16_t id,file;uint8_t attributes; };
bool prepare(const uint8_t* application,uint32_t size);
void release();
int16_t current();
int32_t use(int16_t ref);
int32_t open(int16_t volume,uint32_t directory,const char* path,uint8_t permission,int16_t& ref);
int32_t close(int16_t ref);
int32_t find(uint32_t type,int16_t id,bool currentOnly,Item& out);
int32_t info(uint32_t identity,Item& out,const uint8_t*& name,uint8_t& nameLength);
int32_t read(uint32_t identity,uint8_t* bytes,uint32_t size);
}
