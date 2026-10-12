#ifndef POP2_AMIGA_KEY_MAP_H
#define POP2_AMIGA_KEY_MAP_H
#ifndef POP2_PLATFORM_AMIGA
#include <stdint.h>
#endif
// Physical US key positions adapted from the pinned Alone in the Dark source.
// Character semantics are checked separately against the Macintosh reference.
struct KeyTranslation { uint8_t virtualKey,character,shiftedCharacter; };
inline bool translateAmigaKey(uint8_t raw, KeyTranslation& key)
{
    // Amiga raw keys are physical positions, just like Macintosh ADB virtual
    // keys, but the two matrices use different numbers.  Keep the translation
    // explicit: passing raw values through happened to work for a few letters
    // and silently reported the wrong key for everything else.
    static const uint8_t alphaRaw[] = {
        0x20, 0x35, 0x33, 0x22, 0x12, 0x23, 0x24, 0x25, 0x17, 0x26, 0x27, 0x28,
        0x37, 0x36, 0x18, 0x19, 0x10, 0x13, 0x21, 0x14, 0x16, 0x34, 0x11, 0x32,
        0x15, 0x31
    };
    static const uint8_t alphaMac[] = {
        0x00, 0x0b, 0x08, 0x02, 0x0e, 0x03, 0x05, 0x04, 0x22, 0x26, 0x28, 0x25,
        0x2e, 0x2d, 0x1f, 0x23, 0x0c, 0x0f, 0x01, 0x11, 0x20, 0x09, 0x0d, 0x07,
        0x10, 0x06
    };
    for (uint16_t i = 0; i < 26; ++i) {
        if (raw == alphaRaw[i]) {
            key.virtualKey = alphaMac[i];
            key.character = (uint8_t)('a' + i);
            key.shiftedCharacter = (uint8_t)('A' + i);
            return true;
        }
    }

    static const uint8_t digitMac[] = {
        0x1d, 0x12, 0x13, 0x14, 0x15, 0x17, 0x16, 0x1a, 0x1c, 0x19
    };
    static const uint8_t digitShift[] = {
        ')', '!', '@', '#', '$', '%', '^', '&', '*', '('
    };
    if (raw >= 0x01 && raw <= 0x0a) {
        uint8_t digit = (uint8_t)(raw == 0x0a ? 0 : raw);
        key.virtualKey = digitMac[digit];
        key.character = (uint8_t)('0' + digit);
        key.shiftedCharacter = digitShift[digit];
        return true;
    }

    static const uint8_t keypadRaw[] = {
        0x0f, 0x1d, 0x1e, 0x1f, 0x2d, 0x2e, 0x2f, 0x3d, 0x3e, 0x3f
    };
    static const uint8_t keypadMac[] = {
        0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5b, 0x5c
    };
    for (uint16_t i = 0; i < 10; ++i) {
        if (raw == keypadRaw[i]) {
            key.virtualKey = keypadMac[i];
            key.character = (uint8_t)('0' + i);
            key.shiftedCharacter = key.character;
            return true;
        }
    }

    switch (raw) {
    case 0x00: key = {0x32, '`', '~'}; return true;
    case 0x0b: key = {0x1b, '-', '_'}; return true;
    case 0x0c: key = {0x18, '=', '+'}; return true;
    case 0x0d: key = {0x2a, '\\', '|'}; return true;
    case 0x1a: key = {0x21, '[', '{'}; return true;
    case 0x1b: key = {0x1e, ']', '}'}; return true;
    case 0x29: key = {0x29, ';', ':'}; return true;
    case 0x2a: key = {0x27, '\'', '"'}; return true;
    case 0x38: key = {0x2b, ',', '<'}; return true;
    case 0x39: key = {0x2f, '.', '>'}; return true;
    case 0x3a: key = {0x2c, '/', '?'}; return true;
    case 0x40: key = {0x31, ' ', ' '}; return true;
    case 0x41: key = {0x33, 0x08, 0x08}; return true;
    case 0x42: key = {0x30, 0x09, 0x09}; return true;
    case 0x44: key = {0x24, 0x0d, 0x0d}; return true;
    case 0x45: key = {0x35, 0x1b, 0x1b}; return true;
    case 0x46: key = {0x75, 0x7f, 0x7f}; return true;
    case 0x4c: key = {0x7e, 0x1e, 0x1e}; return true;
    case 0x4d: key = {0x7d, 0x1f, 0x1f}; return true;
    case 0x4e: key = {0x7c, 0x1d, 0x1d}; return true;
    case 0x4f: key = {0x7b, 0x1c, 0x1c}; return true;
    case 0x50: key = {0x7a, 0, 0}; return true;
    case 0x51: key = {0x78, 0, 0}; return true;
    case 0x52: key = {0x63, 0, 0}; return true;
    case 0x53: key = {0x76, 0, 0}; return true;
    case 0x54: key = {0x60, 0, 0}; return true;
    case 0x55: key = {0x61, 0, 0}; return true;
    case 0x56: key = {0x62, 0, 0}; return true;
    case 0x57: key = {0x64, 0, 0}; return true;
    case 0x58: key = {0x65, 0, 0}; return true;
    case 0x59: key = {0x6d, 0, 0}; return true;
    case 0x5f: key = {0x72, 0, 0}; return true;
    case 0x60: case 0x61: key = {0x38, 0, 0}; return true;
    case 0x62: key = {0x39, 0, 0}; return true;
    case 0x63: key = {0x3b, 0, 0}; return true;
    case 0x64: case 0x65: key = {0x3a, 0, 0}; return true;
    case 0x66: case 0x67: key = {0x37, 0, 0}; return true;
    default: return false;
    }
}

#endif
