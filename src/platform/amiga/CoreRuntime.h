#pragma once
namespace CoreRuntime {
bool prepare();
const uint8_t* displayTransfer();
void execute();
bool exitedNormally();
void release();
bool suspendVectors();
void resumeVectors();
const char* error();
}
