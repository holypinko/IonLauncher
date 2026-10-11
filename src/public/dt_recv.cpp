#include "dt_recv.h"
#include "engine/server/player_limits.h"

DECLARE_MODULE(RecvPropHooks)

// clang-format off
DECLARE_HOOK(RecvPropArray2, client.dll + 0x1CEDA0,
[](auto& hook,
	CRecvProp* recv_prop,
	int element_count,
	int element_stride,
	const char* var_name,
	ArrayLengthRecvProxyFn array_length_proxy) -> CRecvProp*
// clang-format on
{
	// Change the amount of elements to account for a bigger player amount
	if (!strcmp(var_name, "\"player_array\""))
		element_count = NEW_MAX_PLAYERS;

	return hook.Original(recv_prop, element_count, element_stride, var_name, array_length_proxy);
})

ON_DLL_LOAD("client.dll", MaxPlayersRecvArrays, [](CModule)
{
    if (!MaxPlayersIncreaseEnabled())
        return;

    DISPATCH_MODULE(RecvPropHooks)
})
