#include "game/server/baseentity.h"

DECLARE_MODULE(ServerEntityListHooks)

// return null when GetEntByIndex is passed an index >= 0x4000
// this is called from exactly 1 script clientcommand that can be given an arbitrary index, and going above 0x4000 crashes
// clang-format off
DECLARE_HOOK(GetEntByIndex, server.dll + 0x2A8A50,
([](auto& hook, int i) -> void*
// clang-format on
{
	const int MAX_ENT_IDX = 0x4000;

	if (i >= MAX_ENT_IDX)
	{
		spdlog::warn("GetEntByIndex {} is out of bounds (max {})", i, MAX_ENT_IDX);
		return nullptr;
	}

	return hook.Original(i);
}))

ON_DLL_LOAD_RELIESON("server.dll", ServerEntityList, ConVar, [](CModule)
{
    DISPATCH_MODULE(ServerEntityListHooks)
})
