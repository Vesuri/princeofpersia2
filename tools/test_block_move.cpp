#include <cstdint>
#include <cstdio>
#include "../src/mac/BlockMoveBytes.h"
#include "../src/mac/BlockMoveChecks.h"
int main() {
    auto checks=BlockMoveChecks::run(BlockMoveBytes::move);
    std::printf("BlockMove checks: %u\n",checks);
    return checks==28388?0:1;
}
