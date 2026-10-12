#ifndef DAJ_MATH_H
#define DAJ_MATH_H
/* uintN_t/intN_t come from the including TU (the framework's compat stdint,
 * etc.) — do NOT include <stdint.h> here, it conflicts with the framework's compat-include. */

/* 16-bit hardware multiply/divide helpers.
 *
 * The 68000 has MULU.W/MULS.W (16x16 -> 32) and DIVU.W/DIVS.W (32 / 16 -> 16-bit
 * quotient + 16-bit remainder) but NO 32-bit multiply or divide — GCC lowers a
 * uint32_t `*`/`/`/`%` into the slow __mulsi3/__udivsi3/__umodsi3 software routines.
 * These helpers force the hardware ops so the codebase carries zero 32-bit software
 * mul/div.  Use ONLY where the operands/quotient provably fit the hardware:
 *   daj_mulu16 / daj_muls16 : both factors < 2^16 (product is a full 32-bit result).
 *   daj_divu16 / daj_divs16 : quotient < 2^16 (else DIVU/DIVS overflow -> V set,
 *                             result undefined) and divisor != 0.
 *   daj_modu16 / daj_mods16 : same range rule; returns the 16-bit remainder.
 * On the SDL/validate host build these are plain C (the host has 32-bit mul/div). */

#if defined(__m68k__)

static inline uint32_t daj_mulu16(uint16_t a, uint16_t b) {
    uint32_t r = a;
    __asm__("mulu.w %1,%0" : "+d"(r) : "d"(b) : "cc");
    return r;                                   /* 16x16 -> 32 */
}
static inline int32_t daj_muls16(int16_t a, int16_t b) {
    int32_t r = a;
    __asm__("muls.w %1,%0" : "+d"(r) : "d"(b) : "cc");
    return r;
}
static inline uint16_t daj_divu16(uint32_t num, uint16_t den) {
    uint32_t r = num;
    __asm__("divu.w %1,%0" : "+d"(r) : "d"(den) : "cc");
    return (uint16_t)r;                         /* low word = quotient */
}
static inline uint16_t daj_modu16(uint32_t num, uint16_t den) {
    uint32_t r = num;
    __asm__("divu.w %1,%0" : "+d"(r) : "d"(den) : "cc");
    return (uint16_t)(r >> 16);                 /* high word = remainder */
}
static inline int16_t daj_divs16(int32_t num, int16_t den) {
    int32_t r = num;
    __asm__("divs.w %1,%0" : "+d"(r) : "d"(den) : "cc");
    return (int16_t)r;
}
static inline int16_t daj_mods16(int32_t num, int16_t den) {
    int32_t r = num;
    __asm__("divs.w %1,%0" : "+d"(r) : "d"(den) : "cc");
    return (int16_t)((uint32_t)r >> 16);
}

#else  /* SDL/validate host — native 32-bit mul/div is fine */

static inline uint32_t daj_mulu16(uint16_t a, uint16_t b) { return (uint32_t)a * b; }
static inline int32_t  daj_muls16(int16_t a, int16_t b)   { return (int32_t)a * b; }
static inline uint16_t daj_divu16(uint32_t num, uint16_t den) { return (uint16_t)(num / den); }
static inline uint16_t daj_modu16(uint32_t num, uint16_t den) { return (uint16_t)(num % den); }
static inline int16_t  daj_divs16(int32_t num, int16_t den)   { return (int16_t)(num / den); }
static inline int16_t  daj_mods16(int32_t num, int16_t den)   { return (int16_t)(num % den); }

#endif
#endif /* DAJ_MATH_H */
