#pragma once

#include "engine/server/player_limits.h"
constexpr int Team_PlayerArray_AddedLength = NEW_MAX_PLAYERS - 32;
constexpr int Team_PlayerArray_AddedSize = PAD_NUMBER(Team_PlayerArray_AddedLength * 8, 4);
constexpr int Team_AddedSize = Team_PlayerArray_AddedSize;
