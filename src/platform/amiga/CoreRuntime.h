#pragma once
namespace CoreRuntime {
bool prepare();
void execute();
void release();
const char* error();
}
