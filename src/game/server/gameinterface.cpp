#include "game/server/gameinterface.h"
#include "engine/server/player_limits.h"
#include "engine/server/sv_client.h"
#include "engine/server/server_limits.h"
#include "tier0/hooks.h"
#include "util/utils.h"
#include "vscript/languages/squirrel_re/squirrel.h"
#include "irecipientfilter.h"
#include "mathlib/vector.h"
#include "tier0/module.h"


DECLARE_MODULE(ServerGameInterfaceHooks)

CServerGameDLL* g_pServerGameDLL;

using CServerGameDLL_OnReceivedSayTextMessage_t = void(__fastcall*)(CServerGameDLL* self, unsigned int senderPlayerId, const char* text,
                                                                bool isTeam);
static CServerGameDLL_OnReceivedSayTextMessage_t CServerGameDLL__OnReceivedSayTextMessage = nullptr;

void CServerGameDLL::OnReceivedSayTextMessage(CServerGameDLL* self, unsigned int senderPlayerId, const char* text, bool isTeam)
{
    if (!CServerGameDLL__OnReceivedSayTextMessage)
    {
        spdlog::error("ChatSendMessage called before original function pointer was set!");
        return;
    }

    CServerGameDLL__OnReceivedSayTextMessage(self, senderPlayerId, text, isTeam);
}
DECLARE_HOOK(CServerGameDLL::OnReceivedSayTextMessage, server.dll + 0x1595C0,
             [](auto& hook, CServerGameDLL* self, unsigned int senderPlayerId, const char* text, bool isTeam)
{
    RemoveAsciiControlSequences(const_cast<char*>(text), true);

    // check chat ratelimits
    if (!g_pServerLimits->CheckChatLimits(&g_pClientArray[senderPlayerId - 1]))
        return;

    SQRESULT result =
        g_pSquirrel[ScriptContext::SERVER]->Call("CServerGameDLL_ProcessMessageStartThread", static_cast<int>(senderPlayerId) - 1, text, isTeam);

    if (result == SQRESULT_ERROR)
        hook.Original(self, senderPlayerId, text, isTeam);
})
ON_DLL_LOAD("server.dll", MiscServerFixes, [](CModule module)
{
	// nop out call to VGUI shutdown since it crashes the game when quitting from the console
	module.Offset(0x154A96).NoOP(5);
})

ON_DLL_LOAD("engine.dll", EngineServerChatHooks, [](CModule module) { g_pServerGameDLL = module.Offset(0x13F0AA98).RCast<CServerGameDLL*>(); })

ON_DLL_LOAD_RELIESON("server.dll", ServerChatHooks, ServerSquirrel, [](CModule module)
{
    DISPATCH_HOOK(ServerGameInterfaceHooks, CServerGameDLL::OnReceivedSayTextMessage)
    CServerGameDLL__OnReceivedSayTextMessage =
        HookSys::GetOriginalFunction<CServerGameDLL_OnReceivedSayTextMessage_t>(HookSys::FindHook("CServerGameDLL::OnReceivedSayTextMessage"));
})
using UserMessageBegin_t = void(__fastcall*)(IRecipientFilter* recipients, const char* messageName, MessageReplayInteraction replayInteraction);
using MessageEnd_t = void(__fastcall*)();
using MessageWriteAngle_t = void(__fastcall*)(float value);
using MessageWriteAngles_t = void(__fastcall*)(const QAngle& value);
using MessageWriteBitVecIntegral_t = void(__fastcall*)(const Vector3D& value);
using MessageWriteBits_t = void(__fastcall*)(const void* data, int bitCount);
using MessageWriteBool_t = void(__fastcall*)(bool value);
using MessageWriteByte_t = void(__fastcall*)(int value);
using MessageWriteChar_t = void(__fastcall*)(int value);
using MessageWriteCoord_t = void(__fastcall*)(float value);
using MessageWriteEHandle_t = void(__fastcall*)(CBaseEntity* entity);
using MessageWriteEntity_t = void(__fastcall*)(int value);
using MessageWriteFloat_t = void(__fastcall*)(float value);
using MessageWriteLong_t = void(__fastcall*)(std::int32_t value);
using MessageWriteSBitLong_t = void(__fastcall*)(std::int32_t value, int bitCount);
using MessageWriteShort_t = void(__fastcall*)(int value);
using MessageWriteString_t = void(__fastcall*)(const char* value);
using MessageWriteUBitLong_t = void(__fastcall*)(std::uint32_t value, int bitCount);
using MessageWriteVec3Coord_t = void(__fastcall*)(const Vector3D& value);
using MessageWriteVec3Normal_t = void(__fastcall*)(const Vector3D& value);
using MessageWriteWord_t = void(__fastcall*)(int value);

UserMessageBegin_t g_pfnUserMessageBegin;
MessageEnd_t g_pfnMessageEnd;
MessageWriteAngle_t g_pfnMessageWriteAngle;
MessageWriteAngles_t g_pfnMessageWriteAngles;
MessageWriteBitVecIntegral_t g_pfnMessageWriteBitVecIntegral;
MessageWriteBits_t g_pfnMessageWriteBits;
MessageWriteBool_t g_pfnMessageWriteBool;
MessageWriteByte_t g_pfnMessageWriteByte;
MessageWriteChar_t g_pfnMessageWriteChar;
MessageWriteCoord_t g_pfnMessageWriteCoord;
MessageWriteEHandle_t g_pfnMessageWriteEHandle;
MessageWriteEntity_t g_pfnMessageWriteEntity;
MessageWriteFloat_t g_pfnMessageWriteFloat;
MessageWriteLong_t g_pfnMessageWriteLong;
MessageWriteSBitLong_t g_pfnMessageWriteSBitLong;
MessageWriteShort_t g_pfnMessageWriteShort;
MessageWriteString_t g_pfnMessageWriteString;
MessageWriteUBitLong_t g_pfnMessageWriteUBitLong;
MessageWriteVec3Coord_t g_pfnMessageWriteVec3Coord;
MessageWriteVec3Normal_t g_pfnMessageWriteVec3Normal;
MessageWriteWord_t g_pfnMessageWriteWord;

void UserMessageBegin(IRecipientFilter& recipients, const char* messageName, const MessageReplayInteraction replayInteraction)
{
    g_pfnUserMessageBegin(&recipients, messageName, replayInteraction);
}

void MessageEnd()
{
    g_pfnMessageEnd();
}

void MessageWriteAngle(const float value)
{
    g_pfnMessageWriteAngle(value);
}

void MessageWriteAngles(const QAngle& value)
{
    g_pfnMessageWriteAngles(value);
}

void MessageWriteBitVecIntegral(const Vector3D& value)
{
    g_pfnMessageWriteBitVecIntegral(value);
}

void MessageWriteBits(const void* data, const int bitCount)
{
    g_pfnMessageWriteBits(data, bitCount);
}

void MessageWriteBool(const bool value)
{
    g_pfnMessageWriteBool(value);
}

void MessageWriteByte(const int value)
{
    g_pfnMessageWriteByte(value);
}

void MessageWriteChar(const int value)
{
    g_pfnMessageWriteChar(value);
}

void MessageWriteCoord(const float value)
{
    g_pfnMessageWriteCoord(value);
}

void MessageWriteEHandle(CBaseEntity* entity)
{
    g_pfnMessageWriteEHandle(entity);
}

void MessageWriteEntity(const int value)
{
    g_pfnMessageWriteEntity(value);
}

void MessageWriteFloat(const float value)
{
    g_pfnMessageWriteFloat(value);
}

void MessageWriteLong(const std::int32_t value)
{
    g_pfnMessageWriteLong(value);
}

void MessageWriteSBitLong(const std::int32_t value, const int bitCount)
{
    g_pfnMessageWriteSBitLong(value, bitCount);
}

void MessageWriteShort(const int value)
{
    g_pfnMessageWriteShort(value);
}

void MessageWriteString(const char* value)
{
    g_pfnMessageWriteString(value);
}

void MessageWriteUBitLong(const std::uint32_t value, const int bitCount)
{
    g_pfnMessageWriteUBitLong(value, bitCount);
}

void MessageWriteVec3Coord(const Vector3D& value)
{
    g_pfnMessageWriteVec3Coord(value);
}

void MessageWriteVec3Normal(const Vector3D& value)
{
    g_pfnMessageWriteVec3Normal(value);
}

void MessageWriteWord(const int value)
{
    g_pfnMessageWriteWord(value);
}

ON_DLL_LOAD("server.dll", ServerUserMessages, [](CModule module)
{
    g_pfnMessageEnd = module.Offset(0x158880).RCast<MessageEnd_t>();
    g_pfnMessageWriteAngle = module.Offset(0x1588B0).RCast<MessageWriteAngle_t>();
    g_pfnMessageWriteAngles = module.Offset(0x158900).RCast<MessageWriteAngles_t>();
    g_pfnMessageWriteBitVecIntegral = module.Offset(0x158940).RCast<MessageWriteBitVecIntegral_t>();
    g_pfnMessageWriteBits = module.Offset(0x1589B0).RCast<MessageWriteBits_t>();
    g_pfnMessageWriteBool = module.Offset(0x158A00).RCast<MessageWriteBool_t>();
    g_pfnMessageWriteByte = module.Offset(0x158A90).RCast<MessageWriteByte_t>();
    g_pfnMessageWriteChar = module.Offset(0x158AD0).RCast<MessageWriteChar_t>();
    g_pfnMessageWriteCoord = module.Offset(0x158B10).RCast<MessageWriteCoord_t>();
    g_pfnMessageWriteEHandle = module.Offset(0x158B60).RCast<MessageWriteEHandle_t>();
    g_pfnMessageWriteEntity = module.Offset(0x158BB0).RCast<MessageWriteEntity_t>();
    g_pfnMessageWriteFloat = module.Offset(0x158BF0).RCast<MessageWriteFloat_t>();
    g_pfnMessageWriteLong = module.Offset(0x158C30).RCast<MessageWriteLong_t>();
    g_pfnMessageWriteSBitLong = module.Offset(0x158C70).RCast<MessageWriteSBitLong_t>();
    g_pfnMessageWriteShort = module.Offset(0x158CC0).RCast<MessageWriteShort_t>();
    g_pfnMessageWriteString = module.Offset(0x158D00).RCast<MessageWriteString_t>();
    g_pfnMessageWriteUBitLong = module.Offset(0x158D40).RCast<MessageWriteUBitLong_t>();
    g_pfnMessageWriteVec3Coord = module.Offset(0x158DA0).RCast<MessageWriteVec3Coord_t>();
    g_pfnMessageWriteVec3Normal = module.Offset(0x158DE0).RCast<MessageWriteVec3Normal_t>();
    g_pfnMessageWriteWord = module.Offset(0x158E20).RCast<MessageWriteWord_t>();
    g_pfnUserMessageBegin = module.Offset(0x15C520).RCast<UserMessageBegin_t>();
})

ON_DLL_LOAD_RELIESON("server.dll", ServerClientCommandKeyValues, ConVar, [](CModule module)
{
	// ret at the start of CServerGameClients::ClientCommandKeyValues as it has no benefit and is forwarded to client (i.e. security issue)
	// this prevents the attack vector of client=>server=>client, however server=>client also has clientside patches
	module.Offset(0x153920).Patch("C3");
})

ON_DLL_LOAD("server.dll", MaxPlayersGameClients, [](CModule module)
{
    if (!MaxPlayersIncreaseEnabled())
        return;

	// patch max players amount
	ChangeOffset<unsigned char>(module.Offset(0x9A44D + 3), NEW_MAX_PLAYERS); // 0x20 (32) => 0x80 (128)
})

DECLARE_HOOK_PROC(ServerCreateInterface, server.dll, CreateInterface, [](auto& hook, const char* pName, int* pReturnCode) -> void*
{
	void* ret = hook.Original(pName, pReturnCode);
	spdlog::info("CreateInterface SERVER {}", pName);

	return ret;
})

ON_DLL_LOAD("server.dll", ServerInterface, [](CModule)
{
	DISPATCH_HOOK(ServerGameInterfaceHooks, ServerCreateInterface)
})
