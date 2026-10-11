#include "engine/sys_dll.h"
#include "tier0/commandline.h"
#include "tier0/vanilla.h"

char* g_pModName = nullptr;

DECLARE_MODULE(SysDllHooks)
// ensure that GetLocalBaseClient().m_bRestrictServerCommands is set correctly, which the return value of this function controls
// this is IsValveMod in source, but we're making it IsRespawnMod now since valve didn't make this one
// clang-format off
DECLARE_HOOK(IsRespawnMod, engine.dll + 0x1C6360,
([](auto& hook, const char* pModName) -> bool // 48 83 EC 28 48 8B 0D ? ? ? ? 48 8D 15 ? ? ? ? E8 ? ? ? ? 85 C0 74 63
// clang-format on
{
	NOTE_UNUSED(hook);
	// somewhat temp, store the modname here, since we don't have a proper ptr in engine to it rn
	size_t iSize = strlen(pModName);
	g_pModName = new char[iSize + 1];
	strcpy(g_pModName, pModName);

	if (g_pVanillaCompatibility->GetVanillaCompatibility())
		return false;

	return (!strcmp("r2", pModName) || !strcmp("r1", pModName)) && !CommandLine()->CheckParm("-norestrictservercommands");
}))

ON_DLL_LOAD("engine.dll", ModCommandRestrictions, [](CModule)
{
    DISPATCH_HOOK(SysDllHooks, IsRespawnMod)
})

DECLARE_HOOK_PROC(EngineCreateInterface, engine.dll, CreateInterface, [](auto& hook, const char* pName, int* pReturnCode) -> void*
{
	void* ret = hook.Original(pName, pReturnCode);
	spdlog::info("CreateInterface ENGINE {}", pName);

	return ret;
})

ON_DLL_LOAD("engine.dll", EngineInterface, [](CModule)
{
	DISPATCH_HOOK(SysDllHooks, EngineCreateInterface)
})
