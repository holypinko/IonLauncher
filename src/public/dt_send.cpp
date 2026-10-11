#include "engine/server/player_limits.h"

DECLARE_MODULE(SendPropHooks)

// clang-format off
DECLARE_HOOK(SendPropArray2, server.dll + 0x12B130,
[](auto& hook, __int64 recvProp, int elements, int flags, const char* name, __int64 proxyFn, unsigned char unk1) -> __int64
// clang-format on
{
	// Change the amount of elements to account for a bigger player amount
	if (!strcmp(name, "\"player_array\""))
		elements = NEW_MAX_PLAYERS;

	return hook.Original(recvProp, elements, flags, name, proxyFn, unk1);
})

ON_DLL_LOAD("server.dll", MaxPlayersSendArrays, [](CModule)
{
    if (!MaxPlayersIncreaseEnabled())
        return;

    DISPATCH_MODULE(SendPropHooks)
})
