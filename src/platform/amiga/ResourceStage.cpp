#ifdef POP2_CORE
#include "SystemWindow.h"
#include "mac/MacFiles.h"
// This port currently uses DOS files, not a resload-backed file provider.
// Keep the provider seam explicit for the shared publication transaction.
namespace FileAccess {
static bool resloadActive() { return false; }
static int32_t resloadStat(const char*,uint32_t&,bool&,bool&) { return MacFiles::unsupported; }
static int32_t resloadReplace(const char*,const uint8_t*,uint32_t) { return MacFiles::unsupported; }
}
#define RESOURCE_STAGE_SYSTEM_WINDOW pop2SystemWindow
#include "mac/ResourceStage.inc"
#endif
