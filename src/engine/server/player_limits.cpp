#include "engine/server/player_limits.h"
#include "tier0/commandline.h"

bool MaxPlayersIncreaseEnabled()
{
	static bool bMaxPlayersIncreaseEnabled = CommandLine()->CheckParm("-experimentalmaxplayersincrease");
	return bMaxPlayersIncreaseEnabled;
}

int GetMaxPlayers()
{
	if (MaxPlayersIncreaseEnabled())
		return NEW_MAX_PLAYERS;

	return 32;
}
