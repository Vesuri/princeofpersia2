#pragma once
namespace CoreRuntime {
bool prepare();
void execute();
void release();
bool suspendVectors();
void resumeVectors();
const char* error();
}
