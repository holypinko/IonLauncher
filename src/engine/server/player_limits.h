#pragma once

#include "tier0/hooks.h"

// Retail supports 32 players; the opt-in extension reserves 64.
inline constexpr int NEW_MAX_PLAYERS = 64;

bool MaxPlayersIncreaseEnabled();
int GetMaxPlayers();

template <class T> void ChangeOffset(CMemory addr, unsigned int offset)
{
	addr.Patch((BYTE*)&offset, sizeof(T));
}
