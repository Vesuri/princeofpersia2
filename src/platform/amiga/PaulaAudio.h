#pragma once
#include "mac/PaulaSample.h"
namespace PaulaAudio {
bool prepare();
void release();
void take();
void restore();
bool start(unsigned channel,const PaulaSample::Layout& layout,uint16_t period,uint16_t volume,uint32_t tailSlot=0,uint32_t sourceOffset=0,const uint8_t* prepared=nullptr);
uint8_t* buffer(unsigned channel);
void stop(unsigned channel);
// Mute and complete the DMA-off wait before replacing/freeing sample bytes.
// Stop still performs the DAC-zero handshake for silence or hardware handoff.
void halt(unsigned channel);
void masterVolume(uint32_t stereo);
void volume(unsigned channel,uint16_t value);
const char* error();
}
