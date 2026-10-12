#pragma once
#include "mac/PaulaSample.h"
namespace PaulaAudio {
bool prepare();
void release();
void take();
void restore();
bool start(unsigned channel,const PaulaSample::Layout& layout,uint16_t period,uint16_t volume,uint32_t tailSlot=0,uint32_t sourceOffset=0);
uint8_t* buffer(unsigned channel);
void stop(unsigned channel);
void volume(unsigned channel,uint16_t value);
const char* error();
}
