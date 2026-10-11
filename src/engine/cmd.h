#pragma once

#include "tier1/convar.h"

bool Cbuf_HasRoomForExecutionMarkers(int marker);
enum class ECommandTarget_t
{
	CBUF_FIRST_PLAYER = 0,
	CBUF_LAST_PLAYER = 1, // MAX_SPLITSCREEN_CLIENTS - 1, MAX_SPLITSCREEN_CLIENTS = 2
	CBUF_SERVER = CBUF_LAST_PLAYER + 1,

	CBUF_COUNT,
};
typedef ECommandTarget_t (*Cbuf_GetCurrentPlayer_t)();
extern Cbuf_GetCurrentPlayer_t Cbuf_GetCurrentPlayer;

typedef void (*Cbuf_AddText_t)(ECommandTarget_t eTarget, const char* text, cmd_source_t source);
extern Cbuf_AddText_t Cbuf_AddText;

typedef void (*Cbuf_Execute_t)();
extern Cbuf_Execute_t Cbuf_Execute;
