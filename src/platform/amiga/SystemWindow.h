#pragma once
// Only from the active user-mode Macintosh service bridge, never an interrupt.
int32_t pop2SystemWindow(int32_t (*operation)(void*),void* context);
