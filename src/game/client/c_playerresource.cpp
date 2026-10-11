#include "game/shared/player_resource.h"

ON_DLL_LOAD_RELIESON("client.dll", MaxPlayersClientResource, MaxPlayersRecvArrays, [](CModule module)
{
    if (!MaxPlayersIncreaseEnabled())
        return;

	constexpr int C_PlayerResource_OriginalSize = 5768;
	constexpr int C_PlayerResource_AddedSize = PlayerResource_TotalSize;
	constexpr int C_PlayerResource_ModifiedSize = C_PlayerResource_OriginalSize + C_PlayerResource_AddedSize;

	// C_PlayerResource class allocation function - allocate a bigger amount to fit all new max player data
	ChangeOffset<unsigned int>(module.Offset(0x164C41 + 1), C_PlayerResource_ModifiedSize);

	// C_PlayerResource::C_PlayerResource - change loop end value
	ChangeOffset<unsigned char>(module.Offset(0x1640C4 + 2), NEW_MAX_PLAYERS - 32);

	// C_PlayerResource::C_PlayerResource - change m_szName address
	ChangeOffset<unsigned int>(
		module.Offset(0x1640D0 + 3), C_PlayerResource_OriginalSize + PlayerResource_Name_Start); // appended to the end of the class

	// C_PlayerResource::C_PlayerResource - change m_szName address
	ChangeOffset<unsigned int>(
		module.Offset(0x1640D0 + 3), C_PlayerResource_OriginalSize + PlayerResource_Name_Start); // appended to the end of the class

	// C_PlayerResource::C_PlayerResource - increase memset length to clean newly allocated data
	ChangeOffset<unsigned int>(module.Offset(0x1640D0 + 3), 2244 + C_PlayerResource_AddedSize);

	// C_PlayerResource::UpdatePlayerName - change m_szName address
	ChangeOffset<unsigned int>(module.Offset(0x16431F + 3), C_PlayerResource_OriginalSize + PlayerResource_Name_Start);

	// C_PlayerResource::GetPlayerName - change m_szName address 1
	ChangeOffset<unsigned int>(module.Offset(0x1645B1 + 3), C_PlayerResource_OriginalSize + PlayerResource_Name_Start);

	// C_PlayerResource::GetPlayerName - change m_szName address 2
	ChangeOffset<unsigned int>(module.Offset(0x1645C0 + 3), C_PlayerResource_OriginalSize + PlayerResource_Name_Start);

	// C_PlayerResource::GetPlayerName - change m_szName address 3
	ChangeOffset<unsigned int>(module.Offset(0x1645DD + 3), C_PlayerResource_OriginalSize + PlayerResource_Name_Start);

	// C_PlayerResource::GetPlayerName internal func - change m_szName address 1
	ChangeOffset<unsigned int>(module.Offset(0x164B71 + 4), C_PlayerResource_OriginalSize + PlayerResource_Name_Start);

	// C_PlayerResource::GetPlayerName internal func - change m_szName address 2
	ChangeOffset<unsigned int>(module.Offset(0x164B9B + 4), C_PlayerResource_OriginalSize + PlayerResource_Name_Start);

	// C_PlayerResource::GetPlayerName2 (?) - change m_szName address 1
	ChangeOffset<unsigned int>(module.Offset(0x164641 + 3), C_PlayerResource_OriginalSize + PlayerResource_Name_Start);

	// C_PlayerResource::GetPlayerName2 (?) - change m_szName address 2
	ChangeOffset<unsigned int>(module.Offset(0x164650 + 3), C_PlayerResource_OriginalSize + PlayerResource_Name_Start);

	// C_PlayerResource::GetPlayerName2 (?) - change m_szName address 3
	ChangeOffset<unsigned int>(module.Offset(0x16466D + 3), C_PlayerResource_OriginalSize + PlayerResource_Name_Start);

	// C_PlayerResource::GetPlayerName internal func - change m_szName2 (?) address 1
	ChangeOffset<unsigned int>(module.Offset(0x164BA3 + 4), C_PlayerResource_OriginalSize + PlayerResource_Name_Start);

	// C_PlayerResource::GetPlayerName internal func - change m_szName2 (?) address 2
	ChangeOffset<unsigned int>(module.Offset(0x164BCE + 4), C_PlayerResource_OriginalSize + PlayerResource_Name_Start);

	// C_PlayerResource::GetPlayerName internal func - change m_szName2 (?) address 3
	ChangeOffset<unsigned int>(module.Offset(0x164BE7 + 4), C_PlayerResource_OriginalSize + PlayerResource_Name_Start);

	// C_PlayerResource::m_szName
	ChangeOffset<unsigned int>(module.Offset(0xc350f8), C_PlayerResource_OriginalSize + PlayerResource_Name_Start);
	ChangeOffset<unsigned short>(module.Offset(0xc350f8 + 4), NEW_MAX_PLAYERS + 1);

	// DT_PlayerResource size
	ChangeOffset<unsigned int>(module.Offset(0x163415 + 6), C_PlayerResource_ModifiedSize);

	// DT_PlayerResource::m_iPing RecvProp
	ChangeOffset<unsigned int>(module.Offset(0x163492 + 2), C_PlayerResource_OriginalSize + PlayerResource_Ping_Start);
	ChangeOffset<unsigned int>(module.Offset(0x1634D6 + 2), C_PlayerResource_OriginalSize + PlayerResource_Ping_Start);
	ChangeOffset<unsigned int>(module.Offset(0x163515 + 5), NEW_MAX_PLAYERS + 1);

	// C_PlayerResource::m_iPing
	ChangeOffset<unsigned int>(module.Offset(0xc35170), C_PlayerResource_OriginalSize + PlayerResource_Ping_Start);
	ChangeOffset<unsigned short>(module.Offset(0xc35170 + 4), NEW_MAX_PLAYERS + 1);

	// DT_PlayerResource::m_iTeam RecvProp
	ChangeOffset<unsigned int>(module.Offset(0x163549 + 2), C_PlayerResource_OriginalSize + PlayerResource_Team_Start);
	ChangeOffset<unsigned int>(module.Offset(0x1635C8 + 2), C_PlayerResource_OriginalSize + PlayerResource_Team_Start);
	ChangeOffset<unsigned int>(module.Offset(0x1635AD + 5), NEW_MAX_PLAYERS + 1);

	// C_PlayerResource::m_iTeam
	ChangeOffset<unsigned int>(module.Offset(0xc351e8), C_PlayerResource_OriginalSize + PlayerResource_Team_Start);
	ChangeOffset<unsigned short>(module.Offset(0xc351e8 + 4), NEW_MAX_PLAYERS + 1);

	// DT_PlayerResource::m_iPRHealth RecvProp
	ChangeOffset<unsigned int>(module.Offset(0x1635F9 + 2), C_PlayerResource_OriginalSize + PlayerResource_PRHealth_Start);
	ChangeOffset<unsigned int>(module.Offset(0x163625 + 2), C_PlayerResource_OriginalSize + PlayerResource_PRHealth_Start);
	ChangeOffset<unsigned int>(module.Offset(0x163675 + 5), NEW_MAX_PLAYERS + 1);

	// C_PlayerResource::m_iPRHealth
	ChangeOffset<unsigned int>(module.Offset(0xc35260), C_PlayerResource_OriginalSize + PlayerResource_PRHealth_Start);
	ChangeOffset<unsigned short>(module.Offset(0xc35260 + 4), NEW_MAX_PLAYERS + 1);

	// DT_PlayerResource::m_bConnected RecvProp
	ChangeOffset<unsigned int>(module.Offset(0x1636A9 + 2), C_PlayerResource_OriginalSize + PlayerResource_Connected_Start);
	ChangeOffset<unsigned int>(module.Offset(0x1636D5 + 2), C_PlayerResource_OriginalSize + PlayerResource_Connected_Start);
	ChangeOffset<unsigned int>(module.Offset(0x163725 + 5), NEW_MAX_PLAYERS + 1);

	// C_PlayerResource::m_bConnected
	ChangeOffset<unsigned int>(module.Offset(0xc352d8), C_PlayerResource_OriginalSize + PlayerResource_Connected_Start);
	ChangeOffset<unsigned short>(module.Offset(0xc352d8 + 4), NEW_MAX_PLAYERS + 1);

	// DT_PlayerResource::m_bAlive RecvProp
	ChangeOffset<unsigned int>(module.Offset(0x163759 + 2), C_PlayerResource_OriginalSize + PlayerResource_Alive_Start);
	ChangeOffset<unsigned int>(module.Offset(0x163785 + 2), C_PlayerResource_OriginalSize + PlayerResource_Alive_Start);
	ChangeOffset<unsigned int>(module.Offset(0x1637D5 + 5), NEW_MAX_PLAYERS + 1);

	// C_PlayerResource::m_bAlive
	ChangeOffset<unsigned int>(module.Offset(0xc35350), C_PlayerResource_OriginalSize + PlayerResource_Alive_Start);
	ChangeOffset<unsigned short>(module.Offset(0xc35350 + 4), NEW_MAX_PLAYERS + 1);

	// DT_PlayerResource::m_boolStats RecvProp
	ChangeOffset<unsigned int>(module.Offset(0x163809 + 2), C_PlayerResource_OriginalSize + PlayerResource_BoolStats_Start);
	ChangeOffset<unsigned int>(module.Offset(0x163835 + 2), C_PlayerResource_OriginalSize + PlayerResource_BoolStats_Start);
	ChangeOffset<unsigned int>(module.Offset(0x163885 + 5), NEW_MAX_PLAYERS + 1);

	// C_PlayerResource::m_boolStats
	ChangeOffset<unsigned int>(module.Offset(0xc353c8), C_PlayerResource_OriginalSize + PlayerResource_BoolStats_Start);
	ChangeOffset<unsigned short>(module.Offset(0xc353c8 + 4), NEW_MAX_PLAYERS + 1);

	// DT_PlayerResource::m_killStats RecvProp
	ChangeOffset<unsigned int>(module.Offset(0x1638B3 + 2), C_PlayerResource_OriginalSize + PlayerResource_KillStats_Start);
	ChangeOffset<unsigned int>(module.Offset(0x1638E5 + 2), C_PlayerResource_OriginalSize + PlayerResource_KillStats_Start);
	ChangeOffset<unsigned int>(module.Offset(0x163935 + 5), PlayerResource_KillStats_Length);

	// C_PlayerResource::m_killStats
	ChangeOffset<unsigned int>(module.Offset(0xc35440), C_PlayerResource_OriginalSize + PlayerResource_KillStats_Start);
	ChangeOffset<unsigned short>(module.Offset(0xc35440 + 4), PlayerResource_KillStats_Length);

	// DT_PlayerResource::m_scoreStats RecvProp
	ChangeOffset<unsigned int>(module.Offset(0x163969 + 2), C_PlayerResource_OriginalSize + PlayerResource_ScoreStats_Start);
	ChangeOffset<unsigned int>(module.Offset(0x163995 + 2), C_PlayerResource_OriginalSize + PlayerResource_ScoreStats_Start);
	ChangeOffset<unsigned int>(module.Offset(0x1639E5 + 5), PlayerResource_ScoreStats_Length);

	// C_PlayerResource::m_scoreStats
	ChangeOffset<unsigned int>(module.Offset(0xc354b8), C_PlayerResource_OriginalSize + PlayerResource_ScoreStats_Start);
	ChangeOffset<unsigned short>(module.Offset(0xc354b8 + 4), PlayerResource_ScoreStats_Length);

	// C_PlayerResource::GetPlayerName - change m_bConnected address
	ChangeOffset<unsigned int>(module.Offset(0x164599 + 3), C_PlayerResource_OriginalSize + PlayerResource_Connected_Start);

	// C_PlayerResource::GetPlayerName2 (?) - change m_bConnected address
	ChangeOffset<unsigned int>(module.Offset(0x164629 + 3), C_PlayerResource_OriginalSize + PlayerResource_Connected_Start);

	// C_PlayerResource::GetPlayerName internal func - change m_bConnected address
	ChangeOffset<unsigned int>(module.Offset(0x164B13 + 3), C_PlayerResource_OriginalSize + PlayerResource_Connected_Start);

	// Some other get name func (that seems to be unused) - change m_bConnected address
	ChangeOffset<unsigned int>(module.Offset(0x164860 + 3), C_PlayerResource_OriginalSize + PlayerResource_Connected_Start);

	// Some other get name func 2 (that seems to be unused too) - change m_bConnected address
	ChangeOffset<unsigned int>(module.Offset(0x164834 + 3), C_PlayerResource_OriginalSize + PlayerResource_Connected_Start);

	*module.Offset(0xC35068).RCast<DWORD*>() = 0;
	auto DT_PlayerResource_Construct = module.Offset(0x163400).RCast<__int64(__fastcall*)()>();
	DT_PlayerResource_Construct();
})
