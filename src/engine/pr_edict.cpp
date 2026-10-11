#include "engine/server/player_limits.h"

ON_DLL_LOAD("engine.dll", MaxPlayersEdictAllocation, [](CModule module)
{
    if (!MaxPlayersIncreaseEnabled())
        return;

	// patch ED_Alloc to change nFirstIndex
	ChangeOffset<int>(module.Offset(0x18F46C + 1), NEW_MAX_PLAYERS + 8 + 1); // original: 41 (sv.GetMaxClients() + 1)
})
