#pragma once

#include "engine/server/player_limits.h"

// Appended resource arrays share offsets across the distinct native class layouts.
constexpr int PlayerResource_Name_Start = 0; // Start of modded allocated space.
constexpr int PlayerResource_Name_Size = ((NEW_MAX_PLAYERS + 1) * 8); // const char* m_szName[MAX_PLAYERS + 1];

constexpr int PlayerResource_Ping_Start = PlayerResource_Name_Start + PlayerResource_Name_Size;
constexpr int PlayerResource_Ping_Size = ((NEW_MAX_PLAYERS + 1) * 4); // int m_iPing[MAX_PLAYERS + 1];

constexpr int PlayerResource_Team_Start = PlayerResource_Ping_Start + PlayerResource_Ping_Size;
constexpr int PlayerResource_Team_Size = ((NEW_MAX_PLAYERS + 1) * 4); // int m_iTeam[MAX_PLAYERS + 1];

constexpr int PlayerResource_PRHealth_Start = PlayerResource_Team_Start + PlayerResource_Team_Size;
constexpr int PlayerResource_PRHealth_Size = ((NEW_MAX_PLAYERS + 1) * 4); // int m_iPRHealth[MAX_PLAYERS + 1];

constexpr int PlayerResource_Connected_Start = PlayerResource_PRHealth_Start + PlayerResource_PRHealth_Size;
constexpr int PlayerResource_Connected_Size = ((NEW_MAX_PLAYERS + 1) * 4); // int (used as a bool) m_bConnected[MAX_PLAYERS + 1];

constexpr int PlayerResource_Alive_Start = PlayerResource_Connected_Start + PlayerResource_Connected_Size;
constexpr int PlayerResource_Alive_Size = ((NEW_MAX_PLAYERS + 1) * 4); // int (used as a bool) m_bAlive[MAX_PLAYERS + 1];

constexpr int PlayerResource_BoolStats_Start = PlayerResource_Alive_Start + PlayerResource_Alive_Size;
constexpr int PlayerResource_BoolStats_Size = ((NEW_MAX_PLAYERS + 1) * 4); // int (used as a bool idk) m_boolStats[MAX_PLAYERS + 1];

constexpr int PlayerResource_KillStats_Start = PlayerResource_BoolStats_Start + PlayerResource_BoolStats_Size;
constexpr int PlayerResource_KillStats_Length = PAD_NUMBER((NEW_MAX_PLAYERS + 1) * 6, 4);
constexpr int PlayerResource_KillStats_Size = (PlayerResource_KillStats_Length * 6); // int m_killStats[MAX_PLAYERS + 1][6];

constexpr int PlayerResource_ScoreStats_Start = PlayerResource_KillStats_Start + PlayerResource_KillStats_Size;
constexpr int PlayerResource_ScoreStats_Length = PAD_NUMBER((NEW_MAX_PLAYERS + 1) * 5, 4);
constexpr int PlayerResource_ScoreStats_Size = (PlayerResource_ScoreStats_Length * 4); // int m_scoreStats[MAX_PLAYERS + 1][5];

// must be the usage of the last field to account for any possible paddings
constexpr int PlayerResource_TotalSize = PlayerResource_ScoreStats_Start + PlayerResource_ScoreStats_Size;
