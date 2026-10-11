#include "engine/server/sv_main.h"
#include "engine/server/player_limits.h"

CGlobalVars* g_pGlobals;

ON_DLL_LOAD("engine.dll", EngineServerGlobals, [](CModule module)
{
    g_pGlobals = module.Offset(0x7C6F70).RCast<CGlobalVars*>();
})

ON_DLL_LOAD("engine.dll", MaxPlayersGameServer, [](CModule module)
{
    if (!MaxPlayersIncreaseEnabled())
        return;

	// patch GetPlayerLimits to ignore the boundary limit
	module.Offset(0x116458).Patch("0xEB"); // jle => jmp
	// patch CGameServer::SpawnServer to change GetMaxClients inline
	ChangeOffset<int>(module.Offset(0x119543 + 2), NEW_MAX_PLAYERS + 8 + 1); // original: 41 (sv.GetMaxClients() + 1)

	// patch CGameServer::SpawnServer to change for loop
	ChangeOffset<unsigned char>(module.Offset(0x11957F + 2), NEW_MAX_PLAYERS); // original: 32

	// patch CGameServer::SpawnServer to change for loop (there are two)
	ChangeOffset<unsigned char>(module.Offset(0x119586 + 2), NEW_MAX_PLAYERS + 1); // original: 33 (32 + 1)
	// patch max players in userinfo stringtable creation loop
	ChangeOffset<unsigned char>(module.Offset(0x114C48 + 2), NEW_MAX_PLAYERS); // original: 32
})
