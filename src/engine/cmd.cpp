#include "engine/cmd.h"
#include "tier1/cvar.h"

static int* g_ExecutionMarkerCount;
// Checks if there's room left for execution markers
bool Cbuf_HasRoomForExecutionMarkers(const int marker)
{
	return (*g_ExecutionMarkerCount + marker) < 2048;
}

ON_DLL_LOAD("engine.dll", CmdBufferSecurity, [](CModule module)
{
    g_ExecutionMarkerCount = module.Offset(0x130DE8F0).RCast<int*>();
    // Permit local client/UI commands while remote server commands remain restricted.
    module.Offset(0x4FB65).Patch("EB 11");
    module.Offset(0x4FBAC).Patch("EB 16");
})

Cbuf_GetCurrentPlayer_t Cbuf_GetCurrentPlayer;
Cbuf_AddText_t Cbuf_AddText;
Cbuf_Execute_t Cbuf_Execute;

ON_DLL_LOAD("engine.dll", CmdBuffer, [](CModule module)
{
    Cbuf_GetCurrentPlayer = module.Offset(0x120630).RCast<Cbuf_GetCurrentPlayer_t>();
    Cbuf_AddText = module.Offset(0x1203B0).RCast<Cbuf_AddText_t>();
    Cbuf_Execute = module.Offset(0x1204B0).RCast<Cbuf_Execute_t>();
})

void ConCommand_cvar_setdefaultvalue(const CCommand& arg)
{
	if (arg.ArgC() < 3)
	{
		spdlog::info("usage: cvar_setdefaultvalue mp_gamemode tdm");
		return;
	}

	ConVar* pCvar = g_pCVar->FindVar(arg.Arg(1));
	if (!pCvar)
	{
		spdlog::info("usage: cvar_setdefaultvalue mp_gamemode tdm");
		return;
	}

	// unfortunately no way for us to not leak memory here, as default value might not be in writeable memory by default
	size_t nLen = strlen(arg.Arg(2));
	char* pBuf = new char[nLen + 1];
	strncpy_s(pBuf, nLen + 1, arg.Arg(2), nLen);

	pCvar->m_pszDefaultValue = pBuf;
}

void ConCommand_cvar_setvalueanddefaultvalue(const CCommand& arg)
{
	if (arg.ArgC() < 3)
	{
		spdlog::info("usage: cvar_setvalueanddefaultvalue mp_gamemode tdm");
		return;
	}

	ConVar* pCvar = g_pCVar->FindVar(arg.Arg(1));
	if (!pCvar)
	{
		spdlog::info("usage: cvar_setvalueanddefaultvalue mp_gamemode tdm");
		return;
	}

	// unfortunately no way for us to not leak memory here, as default value might not be in writeable memory by default
	size_t nLen = strlen(arg.Arg(2));
	char* pBuf = new char[nLen + 1];
	strncpy_s(pBuf, nLen + 1, arg.Arg(2), nLen);

	pCvar->m_pszDefaultValue = pBuf;
	pCvar->SetValue(pCvar->m_pszDefaultValue);
}

void ConCommand_cvar_reset(const CCommand& arg)
{
	if (arg.ArgC() < 2)
	{
		spdlog::info("usage: cvar_reset mp_gamemode");
		return;
	}

	ConVar* pCvar = g_pCVar->FindVar(arg.Arg(1));
	if (!pCvar)
	{
		spdlog::info("usage: cvar_reset mp_gamemode");
		return;
	}

	// reset cvar
	pCvar->SetValue(pCvar->m_pszDefaultValue);
}

ON_DLL_LOAD_RELIESON("engine.dll", ConVarUtilityCommands, (ConCommand, ConVar), [](CModule)
{
	RegisterConCommand(
		"cvar_setdefaultvalue",
		ConCommand_cvar_setdefaultvalue,
		"overwrites the default value of a cvar, for use with script and cvar_reset",
		FCVAR_NONE);
	RegisterConCommand(
		"cvar_setvalueanddefaultvalue",
		ConCommand_cvar_setvalueanddefaultvalue,
		"overwrites the current value and default value of a cvar, for use with script and cvar_reset",
		FCVAR_NONE);
	RegisterConCommand("cvar_reset", ConCommand_cvar_reset, "resets a cvar's value to its default value", FCVAR_NONE);
})
