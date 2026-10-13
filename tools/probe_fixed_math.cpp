#include <cstdio>
#include "mac/FixedMath.h"

int main() {
    unsigned trap, first, second;
    while (std::scanf("%x %x %x", &trap, &first, &second) == 3) {
        if (trap != 0xa84d && trap != 0xa868) return 1;
        const auto a = static_cast<int32_t>(first);
        const auto b = static_cast<int32_t>(second);
        std::printf("%08x\n", trap == 0xa84d ? FixedMath::divide(a, b)
                                           : FixedMath::multiply(a, b));
    }
    return std::feof(stdin) ? 0 : 1;
}
