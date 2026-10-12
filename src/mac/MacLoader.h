#pragma once
// Loader and original instruction entry remain gated by open-work item 6.
class MacLoader {
public:
    bool run() { return false; }
    const char* preparationError() const { return "MAC LOADER / NOT IMPLEMENTED"; }
};
