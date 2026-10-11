#include "engine/client/clientstate.h"

DECLARE_MODULE(ClientEntityParseHooks)
// clang-format off
DECLARE_HOOK(CL_CopyExistingEntity, engine.dll + 0x6F940,
([](auto& hook, void* a1) -> bool
// clang-format on
{
	struct CEntityReadInfo
	{
		BYTE gap[40];
		int nNewEntity;
	};

	CEntityReadInfo* pReadInfo = (CEntityReadInfo*)a1;
	if (pReadInfo->nNewEntity >= 0x1000 || pReadInfo->nNewEntity < 0)
	{
		// Value isn't sanitized in release builds for
		// every game powered by the Source Engine 1
		// causing read/write outside of array bounds.
		// This defect has let to the achievement of a
		// full-chain RCE exploit. We hook and perform
		// sanity checks for the value of m_nNewEntity
		// here to prevent this behavior from happening.
		return false;
	}

	return hook.Original(a1);
}))

ON_DLL_LOAD("engine.dll", ClientEntityParse, [](CModule)
{
    DISPATCH_MODULE(ClientEntityParseHooks)
})
