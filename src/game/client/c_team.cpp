#include "game/shared/team_shared.h"

ON_DLL_LOAD_RELIESON("client.dll", MaxPlayersClientTeam, MaxPlayersClientResource, [](CModule module)
{
    if (!MaxPlayersIncreaseEnabled())
        return;

	constexpr int C_Team_OriginalSize = 3200;
	constexpr int C_Team_AddedSize = Team_AddedSize;
	constexpr int C_Team_ModifiedSize = C_Team_OriginalSize + C_Team_AddedSize;

	// C_Team class allocation function - allocate a bigger amount to fit all new team player data
	ChangeOffset<unsigned int>(module.Offset(0x182321 + 1), C_Team_ModifiedSize);

	// C_Team::C_Team - increase memset length to clean newly allocated data
	ChangeOffset<unsigned int>(module.Offset(0x1804A2 + 2), 256 + C_Team_AddedSize);

	// DT_Team size
	ChangeOffset<unsigned int>(module.Offset(0xC3AA0C), C_Team_ModifiedSize);

	*module.Offset(0xC3AFF8).RCast<DWORD*>() = 0;
	auto DT_Team_Construct = module.Offset(0x17F950).RCast<__int64(__fastcall*)()>();
	DT_Team_Construct();
})
