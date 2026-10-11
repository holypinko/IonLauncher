#include "engine/server/player_limits.h"

ON_DLL_LOAD("engine.dll", MaxPlayersSendTables, [](CModule module)
{
    if (!MaxPlayersIncreaseEnabled())
        return;

    // Rebuild SendTables rather than loading the retail 32-player cache.
    module.Offset(0x75859).Patch("EB");
})
