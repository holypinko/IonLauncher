#include "engine/networkstringtable.h"
#include "engine/server/player_limits.h"

DECLARE_MODULE(StringTableHooks)

bool bWasWritingStringTableSuccessful;

ON_DLL_LOAD("engine.dll", StringTableBaselines, [](CModule module)
{
	// patch to set bWasWritingStringTableSuccessful in CNetworkStringTableContainer::WriteBaselines if it fails
	{
		CMemory writeAddress(&bWasWritingStringTableSuccessful - module.Offset(0x234EDC).GetPtr());

		CMemory addr = module.Offset(0x234ED2);
		addr.Patch("C7 05");
		addr.Offset(2).Patch((BYTE*)&writeAddress, sizeof(writeAddress));

		addr.Offset(6).Patch("00 00 00 00");

		addr.Offset(10).NoOP(5);
	}
})

// clang-format off
DECLARE_HOOK(StringTables_CreateStringTable, engine.dll + 0x22E220,
[](auto& hook, void* thisptr, const char* name, int maxentries, int userdatafixedsize, int userdatanetworkbits, int flags) -> void*
// clang-format on
{
	// Change the amount of entries to account for a bigger player amount
	if (!strcmp(name, "userinfo"))
	{
		int maxPlayersPowerOf2 = 1;
		while (maxPlayersPowerOf2 < NEW_MAX_PLAYERS)
			maxPlayersPowerOf2 <<= 1;

		maxentries = maxPlayersPowerOf2;
	}

	return hook.Original(thisptr, name, maxentries, userdatafixedsize, userdatanetworkbits, flags);
})

ON_DLL_LOAD("engine.dll", MaxPlayersStringTables, [](CModule)
{
    if (!MaxPlayersIncreaseEnabled())
        return;

    DISPATCH_MODULE(StringTableHooks)
})
