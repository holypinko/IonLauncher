#include "engine/host_state.h"
#include "tier1/convar.h"
void ConCommand_force_newgame(const CCommand& arg)
{
	if (arg.ArgC() < 2)
		return;

	g_pHostState->m_iNextState = HostState_t::HS_NEW_GAME;
	strncpy(g_pHostState->m_levelName, arg.Arg(1), sizeof(g_pHostState->m_levelName));
}

ON_DLL_LOAD_RELIESON("engine.dll", HostCommands, ConCommand, [](CModule)
{
	RegisterConCommand(
		"force_newgame",
		ConCommand_force_newgame,
		"forces a map load through directly setting g_pHostState->m_iNextState to HS_NEW_GAME",
		FCVAR_NONE);
})
