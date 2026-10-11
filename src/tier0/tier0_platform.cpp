#include "tier0/commandline.h"
#include "tier0/memalloc.h"
#include "tier0/platform.h"
#include "tier0/threadtools_internal.h"

Plat_FloatTime_t g_PlatFloatTime;

ON_DLL_LOAD("tier0.dll", Tier0GameFuncs, [](CModule module)
{
	// Preserve the allocator, debug-name hook and native export binding order.
	TryCreateGlobalMemAlloc();
	InitialiseThreadDebugNameHook();
	CommandLine = module.GetExportedFunction("CommandLine").RCast<CommandLine_t>();
	g_PlatFloatTime = module.GetExportedFunction("Plat_FloatTime").RCast<Plat_FloatTime_t>();
	InitialiseThreadCheckBindings(module);
})
