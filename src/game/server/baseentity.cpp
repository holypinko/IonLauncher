#include "game/server/baseentity.h"

ON_DLL_LOAD_RELIESON("server.dll", ServerHitscanTrace, ConVar, [](CModule module)
{
    // Ignore hitscan traces ending outside maximum map coordinates instead of terminating the server.
    module.Offset(0x43D4D8).NoOP(6);
})
