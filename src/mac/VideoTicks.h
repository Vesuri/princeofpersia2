#ifndef AMIGAXDEV_VIDEO_TICKS_H
#define AMIGAXDEV_VIDEO_TICKS_H
#ifndef __mc68000__
#include <stdint.h>
#endif
namespace VideoTicks {
// Existing Alone in the Dark PAL/NTSC field conversion. Phase is in [0,4].
inline uint16_t fieldDelta(bool pal,uint16_t& phase) {
    if(pal && ++phase==5) { phase=0;return 2; }
    return 1;
}
// TickCount between PAL interrupts. fieldTick/phase describe the start of the
// current field; microseconds is its clamped nominal position in [0,19999].
// Retain the fractional phase so a five-tick wait does not repeatedly miss
// tick 5 at the field-only sequence 0,1,2,3,4,6. Unsigned addition wraps Ticks.
inline uint32_t withinPALField(uint32_t fieldTick,uint16_t phase,uint16_t microseconds) {
    return fieldTick+(uint32_t(phase)*20000UL+uint32_t(microseconds)*6>=100000UL);
}
}
#endif
