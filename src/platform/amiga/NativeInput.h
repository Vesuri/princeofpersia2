#ifndef POP2_NATIVE_INPUT_H
#define POP2_NATIVE_INPUT_H
#include "mac/InputState.h"
namespace NativeInput {
bool initialize();
void shutdown();
void field(uint32_t ticks);
uint32_t ticks();
void keyMap(uint8_t* output);
bool event(uint16_t mask,bool remove,uint32_t ticks,InputState::Event& output);
void flush(uint16_t which,uint16_t stop);
const char* error();
// Diagnostic ingress uses the same raw edge queue as the CIA keyboard ISR.
void inject(uint8_t raw,bool down);
}
#endif
