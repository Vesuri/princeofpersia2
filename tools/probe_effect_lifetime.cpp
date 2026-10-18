// Original effect descriptors arrive through stdin; no original data is embedded.
#include <cstdio>
#include <cstdint>
#include "mac/HalestormEngine.h"

int main() {
    uint32_t size, rate;
    static const uint8_t dummy = 0;
    while (std::scanf("%u %u", &size, &rate) == 2) {
        HalestormEngine engine;
        if (engine.configure(5, 5, 1) || engine.effectOn(&dummy, size, rate, 1))
            return 2;
        unsigned blocks = 0;
        while (engine.voices.effect.duration >= 0 && blocks < 32768) {
            engine.advance(); ++blocks;
        }
        if (engine.voices.effect.duration >= 0 || engine.completed != 32)
            return 3;
        // Logical cursor advancement never reads sample bytes.
        std::printf("%u\n", blocks);
    }
}
