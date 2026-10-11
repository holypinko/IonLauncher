#include "game/shared/player_resource.h"

ON_DLL_LOAD_RELIESON("server.dll", MaxPlayersServerResource, MaxPlayersSendArrays, [](CModule module)
{
    if (!MaxPlayersIncreaseEnabled())
        return;

	constexpr int CPlayerResource_OriginalSize = 4776;
	constexpr int CPlayerResource_AddedSize = PlayerResource_TotalSize;
	constexpr int CPlayerResource_ModifiedSize = CPlayerResource_OriginalSize + CPlayerResource_AddedSize;

	// CPlayerResource class allocation function - allocate a bigger amount to fit all new max player data
	ChangeOffset<unsigned int>(module.Offset(0x5C560A + 1), CPlayerResource_ModifiedSize);

	// DT_PlayerResource::m_iPing SendProp
	ChangeOffset<unsigned int>(module.Offset(0x5C5059 + 2), CPlayerResource_OriginalSize + PlayerResource_Ping_Start);
	ChangeOffset<unsigned int>(module.Offset(0x5C50A8 + 2), CPlayerResource_OriginalSize + PlayerResource_Ping_Start);
	ChangeOffset<unsigned int>(module.Offset(0x5C50E2 + 4), NEW_MAX_PLAYERS + 1);

	// DT_PlayerResource::m_iPing DataMap
	ChangeOffset<unsigned int>(module.Offset(0xB94598), CPlayerResource_OriginalSize + PlayerResource_Ping_Start);
	ChangeOffset<unsigned short>(module.Offset(0xB9459C), NEW_MAX_PLAYERS + 1);
	ChangeOffset<unsigned short>(module.Offset(0xB945C0), PlayerResource_Ping_Size);

	// DT_PlayerResource::m_iTeam SendProp
	ChangeOffset<unsigned int>(module.Offset(0x5C5110 + 2), CPlayerResource_OriginalSize + PlayerResource_Team_Start);
	ChangeOffset<unsigned int>(module.Offset(0x5C519C + 2), CPlayerResource_OriginalSize + PlayerResource_Team_Start);
	ChangeOffset<unsigned int>(module.Offset(0x5C517E + 4), NEW_MAX_PLAYERS + 1);

	// DT_PlayerResource::m_iTeam DataMap
	ChangeOffset<unsigned int>(module.Offset(0xB94600), CPlayerResource_OriginalSize + PlayerResource_Team_Start);
	ChangeOffset<unsigned short>(module.Offset(0xB94604), NEW_MAX_PLAYERS + 1);
	ChangeOffset<unsigned short>(module.Offset(0xB94628), PlayerResource_Team_Size);

	// DT_PlayerResource::m_iPRHealth SendProp
	ChangeOffset<unsigned int>(module.Offset(0x5C51C0 + 2), CPlayerResource_OriginalSize + PlayerResource_PRHealth_Start);
	ChangeOffset<unsigned int>(module.Offset(0x5C5204 + 2), CPlayerResource_OriginalSize + PlayerResource_PRHealth_Start);
	ChangeOffset<unsigned int>(module.Offset(0x5C523E + 4), NEW_MAX_PLAYERS + 1);

	// DT_PlayerResource::m_iPRHealth DataMap
	ChangeOffset<unsigned int>(module.Offset(0xB94668), CPlayerResource_OriginalSize + PlayerResource_PRHealth_Start);
	ChangeOffset<unsigned short>(module.Offset(0xB9466C), NEW_MAX_PLAYERS + 1);
	ChangeOffset<unsigned short>(module.Offset(0xB94690), PlayerResource_PRHealth_Size);

	// DT_PlayerResource::m_bConnected SendProp
	ChangeOffset<unsigned int>(module.Offset(0x5C526C + 2), CPlayerResource_OriginalSize + PlayerResource_Connected_Start);
	ChangeOffset<unsigned int>(module.Offset(0x5C52B4 + 2), CPlayerResource_OriginalSize + PlayerResource_Connected_Start);
	ChangeOffset<unsigned int>(module.Offset(0x5C52EE + 4), NEW_MAX_PLAYERS + 1);

	// DT_PlayerResource::m_bConnected DataMap
	ChangeOffset<unsigned int>(module.Offset(0xB946D0), CPlayerResource_OriginalSize + PlayerResource_Connected_Start);
	ChangeOffset<unsigned short>(module.Offset(0xB946D4), NEW_MAX_PLAYERS + 1);
	ChangeOffset<unsigned short>(module.Offset(0xB946F8), PlayerResource_Connected_Size);

	// DT_PlayerResource::m_bAlive SendProp
	ChangeOffset<unsigned int>(module.Offset(0x5C5321 + 2), CPlayerResource_OriginalSize + PlayerResource_Alive_Start);
	ChangeOffset<unsigned int>(module.Offset(0x5C5364 + 2), CPlayerResource_OriginalSize + PlayerResource_Alive_Start);
	ChangeOffset<unsigned int>(module.Offset(0x5C539E + 4), NEW_MAX_PLAYERS + 1);

	// DT_PlayerResource::m_bAlive DataMap
	ChangeOffset<unsigned int>(module.Offset(0xB94738), CPlayerResource_OriginalSize + PlayerResource_Alive_Start);
	ChangeOffset<unsigned short>(module.Offset(0xB9473C), NEW_MAX_PLAYERS + 1);
	ChangeOffset<unsigned short>(module.Offset(0xB94760), PlayerResource_Alive_Size);

	// DT_PlayerResource::m_boolStats SendProp
	ChangeOffset<unsigned int>(module.Offset(0x5C53CC + 2), CPlayerResource_OriginalSize + PlayerResource_BoolStats_Start);
	ChangeOffset<unsigned int>(module.Offset(0x5C5414 + 2), CPlayerResource_OriginalSize + PlayerResource_BoolStats_Start);
	ChangeOffset<unsigned int>(module.Offset(0x5C544E + 4), NEW_MAX_PLAYERS + 1);

	// DT_PlayerResource::m_boolStats DataMap
	ChangeOffset<unsigned int>(module.Offset(0xB947A0), CPlayerResource_OriginalSize + PlayerResource_BoolStats_Start);
	ChangeOffset<unsigned short>(module.Offset(0xB947A4), NEW_MAX_PLAYERS + 1);
	ChangeOffset<unsigned short>(module.Offset(0xB947C8), PlayerResource_BoolStats_Size);

	// DT_PlayerResource::m_killStats SendProp
	ChangeOffset<unsigned int>(module.Offset(0x5C547C + 2), CPlayerResource_OriginalSize + PlayerResource_KillStats_Start);
	ChangeOffset<unsigned int>(module.Offset(0x5C54E2 + 2), CPlayerResource_OriginalSize + PlayerResource_KillStats_Start);
	ChangeOffset<unsigned int>(module.Offset(0x5C54FE + 4), PlayerResource_KillStats_Length);

	// DT_PlayerResource::m_killStats DataMap
	ChangeOffset<unsigned int>(module.Offset(0xB94808), CPlayerResource_OriginalSize + PlayerResource_KillStats_Start);
	ChangeOffset<unsigned short>(module.Offset(0xB9480C), PlayerResource_KillStats_Length);
	ChangeOffset<unsigned short>(module.Offset(0xB94830), PlayerResource_KillStats_Size);

	// DT_PlayerResource::m_scoreStats SendProp
	ChangeOffset<unsigned int>(module.Offset(0x5C5528 + 2), CPlayerResource_OriginalSize + PlayerResource_ScoreStats_Start);
	ChangeOffset<unsigned int>(module.Offset(0x5C5576 + 2), CPlayerResource_OriginalSize + PlayerResource_ScoreStats_Start);
	ChangeOffset<unsigned int>(module.Offset(0x5C5584 + 4), PlayerResource_ScoreStats_Length);

	// DT_PlayerResource::m_scoreStats DataMap
	ChangeOffset<unsigned int>(module.Offset(0xB94870), CPlayerResource_OriginalSize + PlayerResource_ScoreStats_Start);
	ChangeOffset<unsigned short>(module.Offset(0xB94874), PlayerResource_ScoreStats_Length);
	ChangeOffset<unsigned short>(module.Offset(0xB94898), PlayerResource_ScoreStats_Size);

	// CPlayerResource::UpdatePlayerData - m_bConnected
	ChangeOffset<unsigned int>(module.Offset(0x5C66EE + 4), CPlayerResource_OriginalSize + PlayerResource_Connected_Start);
	ChangeOffset<unsigned int>(module.Offset(0x5C672E + 4), CPlayerResource_OriginalSize + PlayerResource_Connected_Start);

	// CPlayerResource::UpdatePlayerData - m_iPing
	ChangeOffset<unsigned int>(module.Offset(0x5C6394 + 4), CPlayerResource_OriginalSize + PlayerResource_Ping_Start);
	ChangeOffset<unsigned int>(module.Offset(0x5C63DB + 4), CPlayerResource_OriginalSize + PlayerResource_Ping_Start);

	// CPlayerResource::UpdatePlayerData - m_iTeam
	ChangeOffset<unsigned int>(module.Offset(0x5C63FD + 4), CPlayerResource_OriginalSize + PlayerResource_Team_Start);
	ChangeOffset<unsigned int>(module.Offset(0x5C6442 + 4), CPlayerResource_OriginalSize + PlayerResource_Team_Start);

	// CPlayerResource::UpdatePlayerData - m_iPRHealth
	ChangeOffset<unsigned int>(module.Offset(0x5C645B + 4), CPlayerResource_OriginalSize + PlayerResource_PRHealth_Start);
	ChangeOffset<unsigned int>(module.Offset(0x5C64A0 + 4), CPlayerResource_OriginalSize + PlayerResource_PRHealth_Start);

	// CPlayerResource::UpdatePlayerData - m_bConnected
	ChangeOffset<unsigned int>(module.Offset(0x5C64AA + 4), CPlayerResource_OriginalSize + PlayerResource_Connected_Start);
	ChangeOffset<unsigned int>(module.Offset(0x5C64F0 + 4), CPlayerResource_OriginalSize + PlayerResource_Connected_Start);

	// CPlayerResource::UpdatePlayerData - m_bAlive
	ChangeOffset<unsigned int>(module.Offset(0x5C650A + 4), CPlayerResource_OriginalSize + PlayerResource_Alive_Start);
	ChangeOffset<unsigned int>(module.Offset(0x5C654F + 4), CPlayerResource_OriginalSize + PlayerResource_Alive_Start);

	// CPlayerResource::UpdatePlayerData - m_boolStats
	ChangeOffset<unsigned int>(module.Offset(0x5C6557 + 4), CPlayerResource_OriginalSize + PlayerResource_BoolStats_Start);
	ChangeOffset<unsigned int>(module.Offset(0x5C65A5 + 4), CPlayerResource_OriginalSize + PlayerResource_BoolStats_Start);

	// CPlayerResource::UpdatePlayerData - m_scoreStats
	ChangeOffset<unsigned int>(module.Offset(0x5C65C2 + 3), CPlayerResource_OriginalSize + PlayerResource_ScoreStats_Start);
	ChangeOffset<unsigned int>(module.Offset(0x5C65E3 + 4), CPlayerResource_OriginalSize + PlayerResource_ScoreStats_Start);

	// CPlayerResource::UpdatePlayerData - m_killStats
	ChangeOffset<unsigned int>(module.Offset(0x5C6654 + 3), CPlayerResource_OriginalSize + PlayerResource_KillStats_Start);
	ChangeOffset<unsigned int>(module.Offset(0x5C665B + 3), CPlayerResource_OriginalSize + PlayerResource_KillStats_Start);

	*module.Offset(0x14E7390).RCast<DWORD*>() = 0;
	auto DT_PlayerResource_Construct = module.Offset(0x5C4FE0).RCast<__int64(__fastcall*)()>();
	DT_PlayerResource_Construct();
})
