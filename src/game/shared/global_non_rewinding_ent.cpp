#include "engine/server/player_limits.h"

ON_DLL_LOAD("server.dll", MaxPlayersGlobalEntity, [](CModule module)
{
    if (!MaxPlayersIncreaseEnabled())
        return;

	// patch SpawnGlobalNonRewinding to change forced edict index
	ChangeOffset<unsigned char>(module.Offset(0x2BC403 + 2), NEW_MAX_PLAYERS + 1); // original: 33 (32 + 1)
})
