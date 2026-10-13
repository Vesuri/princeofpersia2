#include <cstdio>
#include "mac/GWorld8.h"

int main() {
    int top, left, bottom, right;
    while (std::scanf("%d %d %d %d", &top, &left, &bottom, &right) == 4) {
        GWorld8::Rect bounds{int16_t(top), int16_t(left), int16_t(bottom), int16_t(right)};
        if (top || left || !(((right == 512 || right == 510) && bottom == 384)
                            || (right == 510 && bottom == 87))) return 1;
        uint8_t port[108], map[50];
        GWorld8::port(port, 0x1000, 0x2000, 0x3000, 0x4000, 0x5000, 0x6000, 0x7000, bounds);
        GWorld8::pixmap(map, 0x8000, 0x9000, bounds, {528, uint32_t(528*bottom)});
        if (std::fwrite(port, 1, sizeof port, stdout) != sizeof port
            || std::fwrite(map, 1, sizeof map, stdout) != sizeof map) return 1;
    }
    return std::feof(stdin) ? 0 : 1;
}
