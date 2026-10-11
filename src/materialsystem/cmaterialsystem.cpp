#include "tier0/callbacks.h"

ON_DLL_LOAD_CLIENT_RELIESON("materialsystem_dx11.dll", CMaterialSystemFindMaterial, KeyValuesDiskVMTLoading, [](CModule module)
{
	// CMaterialSystem::FindMaterial must not call the cache path after disk VMT fallback is enabled.
	module.Offset(0x5F55A).NoOP(5);
})
