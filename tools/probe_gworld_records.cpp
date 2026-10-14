#include <cstdio>
#include "mac/GWorld8.h"

int main() {
    int top, left, bottom, right;
    while (std::scanf("%d %d %d %d", &top, &left, &bottom, &right) == 4) {
        GWorld8::Rect bounds{int16_t(top), int16_t(left), int16_t(bottom), int16_t(right)};
        uint8_t port[108], map[50];
        GWorld8::port(port, 0x1000, 0x2000, 0x3000, 0x4000, 0x5000, 0x6000, 0x7000, bounds);
        GWorld8::Layout layout;
        if(!GWorld8::layout(bounds,layout,16))return 1;
        GWorld8::pixmap(map, 0x8000, 0x9000, bounds, layout);
        if (std::fwrite(port, 1, sizeof port, stdout) != sizeof port
            || std::fwrite(map, 1, sizeof map, stdout) != sizeof map) return 1;
    }
    return std::feof(stdin) ? 0 : 1;
}
