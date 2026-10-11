#include "game/shared/team_shared.h"

ON_DLL_LOAD_RELIESON("server.dll", MaxPlayersServerTeam, MaxPlayersServerResource, [](CModule module)
{
    if (!MaxPlayersIncreaseEnabled())
        return;

	constexpr int CTeam_OriginalSize = 3336;
	constexpr int CTeam_AddedSize = Team_AddedSize;
	constexpr int CTeam_ModifiedSize = CTeam_OriginalSize + CTeam_AddedSize;

	// CTeam class allocation function - allocate a bigger amount to fit all new team player data
	ChangeOffset<unsigned int>(module.Offset(0x23924A + 1), CTeam_ModifiedSize);

	// CTeam::CTeam - increase memset length to clean newly allocated data
	ChangeOffset<unsigned int>(module.Offset(0x2395AE + 2), 256 + CTeam_AddedSize);

	*module.Offset(0xC945A0).RCast<DWORD*>() = 0;
	auto DT_Team_Construct = module.Offset(0x238F50).RCast<__int64(__fastcall*)()>();
	DT_Team_Construct();
})
