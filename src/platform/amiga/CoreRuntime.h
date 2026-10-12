#pragma once
namespace CoreRuntime {
bool prepare();
const uint8_t* displayTransfer();
void execute();
void release();
bool suspendVectors();
void resumeVectors();
const char* error();
}
