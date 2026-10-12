#ifndef POP2_NATIVE_UI_H
#define POP2_NATIVE_UI_H
#include "NativeDisplay.h"
namespace NativeUI {
bool prepare(MacHeap& heap,uint8_t* low,NativeDisplay::Valid valid,NativeDisplay::Resource resource);
bool handles(uint16_t trap);
bool dispatch(uint16_t trap,uint32_t* registers,uint8_t* arguments,uint32_t& consumed);
bool replacement(uint16_t selector,uint32_t* registers,uint8_t* arguments);
const char* error();
}
#endif
