#include "engine/sys_engine.h"
#include "dedicated/dedicated.h"

CEngine* g_pEngine;

ON_DLL_LOAD("engine.dll", EngineSystem, [](CModule module)
{
    g_pEngine = module.Offset(0x7D70C8).Deref().RCast<CEngine*>();
})

DECLARE_MODULE(EngineFrameHooks)
static float* s_pHostStateIntervalPerTick;
static float* s_pHostRemainder;
static ConVar** p_Cvar_fps_max;

DECLARE_HOOK(CEngine::FilterTime, engine.dll + 0x1C84A0,
([](auto& hook, void* self, float dt, float* flMinFrameTime) -> bool
{
	if (IsDedicatedServer()) {
		const float nextTick = *s_pHostStateIntervalPerTick - *s_pHostRemainder;
		*flMinFrameTime = nextTick;
		return dt >= nextTick;
	}

	bool result = hook.Original(self, dt, flMinFrameTime);

	constexpr float retailMaxFrameRate = 144.0f;
	float requestedFrameRate = (*p_Cvar_fps_max)->GetFloat();

	if (requestedFrameRate > retailMaxFrameRate && *flMinFrameTime == 1.0f / retailMaxFrameRate)
	{
		float maxFrameRate = fminf(requestedFrameRate, 1000.0f);
		float minFrameTime = 1.0f / maxFrameRate;
		*flMinFrameTime = minFrameTime;
		return result || dt >= minFrameTime;
	}

	return result;
}))

ON_DLL_LOAD_RELIESON("engine.dll", EngineFrameLimits, ConVar, [](CModule module)
{
	s_pHostStateIntervalPerTick = module.Offset(0x7CB418).RCast<float*>();
	s_pHostRemainder = module.Offset(0x1315ADBC).RCast<float*>();
	p_Cvar_fps_max = module.Offset(0x13F0CA08).RCast<ConVar**>();
    DISPATCH_MODULE(EngineFrameHooks)
})
