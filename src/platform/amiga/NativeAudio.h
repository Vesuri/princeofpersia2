#pragma once
#include "mac/HalestormEngine.h"
namespace NativeAudio {
using Validate=bool(*)(uint32_t,uint32_t);
bool prepare(const uint8_t* pitch,HalestormEngine::Lookup lookup,Validate validate);
void release();
void field();
void take();
void restore();
bool sync();
bool servicePending();
// retry resumes the same caller after queued original callbacks have run.
bool dispatch(uint32_t selector,uint32_t argument,uint32_t& result,bool& retry);
bool callback(uint32_t& code,uint32_t& context);
const char* error();
const HalestormEngine& state();
#ifdef POP2_AUDIO_PROBE
void replay(bool enabled);
bool advanceTo(uint32_t clock);
#endif
}
