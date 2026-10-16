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
// Exact equivalent of withinPALField for a stable PAL beam with HP < 227.
// Derive each boundary from the nominal 64 us line / 227 positions used above.
// Callers retain responsibility for ambiguous beam/interrupt snapshots.
inline uint32_t withinPALBeam(uint32_t fieldTick,uint16_t phase,uint16_t line,uint16_t hp) {
    static const uint32_t boundary[5]={0x10460,0x0d04f,0x09c39,0x06828,0x03416};
    return fieldTick+((uint32_t(line)<<8|hp)>=boundary[phase]);
}
// First valid beam position whose clamped nominal microsecond value reaches
// the deadline. Invert floor(HP*64/227) with a ceiling; no runtime division by
// 227 is needed for a due/not-due query. Callers retain stable snapshot guards.
inline uint32_t deadlinePALBeam(uint32_t microseconds) {
    if(microseconds>19999)return 0xffffffffUL;
    return ((microseconds>>6)<<8)|(((microseconds&63)*227+63)>>6);
}

}
#endif
