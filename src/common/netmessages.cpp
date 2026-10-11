#include "common/netmessages.h"
#include "tier0/callbacks.h"
#include "engine/client/clientstate.h"
#include "engine/server/server_limits.h"
#include "tier0/threadtools.h"
#include "tier0/vanilla.h"
#include "tier1/cvar.h"
#include "util/utils.h"

DECLARE_MODULE(NetMessagesHooks)

#include <cstdio>

NET_SignonState::NET_SignonState()
	: m_nSignonState(eSignonState::NONE),
	  m_nSpawnCount(0),
	  m_szMapName{},
	  m_szGameMode{},
	  m_nPlaylistVersion(0),
	  m_bSendPlaylists(false),
	  m_szPlaylistName{}
{
	m_nGroup = 0;
	m_bReliable = true;
	m_NetChannel = nullptr;
	m_pMessageHandler = nullptr;
}

NET_SignonState::NET_SignonState(eSignonState state, std::int32_t spawnCount)
	: NET_SignonState()
{
	m_nSignonState = state;
	m_nSpawnCount = spawnCount;
}

bool NET_SignonState::Process()
{
	return m_pMessageHandler->ProcessSignonState(this);
}

bool NET_SignonState::ReadFromBuffer(bf_read* buffer)
{
	m_nSignonState = static_cast<eSignonState>(buffer->ReadByte());
	m_nSpawnCount = buffer->ReadLong();
	buffer->ReadString(m_szMapName, sizeof(m_szMapName));
	buffer->ReadString(m_szGameMode, sizeof(m_szGameMode));
	m_bSendPlaylists = buffer->ReadByte() != 0;
	m_nPlaylistVersion = buffer->ReadLong();
	buffer->ReadString(m_szPlaylistName, sizeof(m_szPlaylistName));
	return !buffer->IsOverflowed();
}

bool NET_SignonState::WriteToBuffer(bf_write* buffer)
{
	buffer->WriteByte(static_cast<int>(m_nSignonState));
	buffer->WriteLong(m_nSpawnCount);
	buffer->WriteString(m_szMapName);
	buffer->WriteString(m_szGameMode);
	buffer->WriteByte(m_bSendPlaylists);
	buffer->WriteLong(m_nPlaylistVersion);
	buffer->WriteString(m_szPlaylistName);
	return !buffer->IsOverflowed();
}

const char* NET_SignonState::ToString() const
{
	static char text[1024];
	std::snprintf(text, sizeof(text), "%s: state %i, count %i", GetName(), static_cast<int>(m_nSignonState), m_nSpawnCount);
	return text;
}

using ClientTickProcess_t = bool (*)(CLC_ClientTick*);
using ClientTickRead_t = bool (*)(CLC_ClientTick*, bf_read*);
using ClientTickWrite_t = bool (*)(CLC_ClientTick*, bf_write*);
using ClientTickToString_t = const char* (*)(const CLC_ClientTick*);

ClientTickProcess_t CLC_ClientTick__Process;
ClientTickRead_t CLC_ClientTick__ReadFromBuffer;
ClientTickWrite_t CLC_ClientTick__WriteToBuffer;
ClientTickToString_t CLC_ClientTick__ToString;

CLC_ClientTick::CLC_ClientTick()
	: m_nDeltaTick(0), m_nStringTableTick(0), m_flFrameTime(0), m_flFrameTimeStdDeviation(0), m_nServerCPU(0)
{
	m_nGroup = 0;
	m_bReliable = false;
	m_NetChannel = nullptr;
	m_pMessageHandler = nullptr;
}

bool CLC_ClientTick::Process()
{
	return CLC_ClientTick__Process(this);
}

bool CLC_ClientTick::ReadFromBuffer(bf_read* buffer)
{
	return CLC_ClientTick__ReadFromBuffer(this, buffer);
}

bool CLC_ClientTick::WriteToBuffer(bf_write* buffer)
{
	return CLC_ClientTick__WriteToBuffer(this, buffer);
}

const char* CLC_ClientTick::ToString() const
{
	return CLC_ClientTick__ToString(this);
}

ON_DLL_LOAD_CLIENT("engine.dll", ClientTickMessageMethods, [](CModule module)
{
	CLC_ClientTick__Process = module.Offset(0x75CC0).RCast<ClientTickProcess_t>();
	CLC_ClientTick__ReadFromBuffer = module.Offset(0x220AF0).RCast<ClientTickRead_t>();
	CLC_ClientTick__WriteToBuffer = module.Offset(0x22AA80).RCast<ClientTickWrite_t>();
	CLC_ClientTick__ToString = module.Offset(0x229540).RCast<ClientTickToString_t>();
})
DECLARE_HOOK(NET_SignonState::ReadFromBuffer, engine.dll + 0x222630, [](auto& hook, NET_SignonState* message, bf_read* buffer) -> bool
{
	if (g_pVanillaCompatibility->GetVanillaCompatibility())
		return hook.Original(message, buffer);

	return message->NET_SignonState::ReadFromBuffer(buffer);
})

DECLARE_HOOK(NET_SignonState::WriteToBuffer, engine.dll + 0x22B0E0, [](auto& hook, NET_SignonState* message, bf_write* buffer) -> bool
{
	if (g_pVanillaCompatibility->GetVanillaCompatibility())
		return hook.Original(message, buffer);

	if (!g_bSendingClientSignonCapability)
		return message->NET_SignonState::WriteToBuffer(buffer);

	const bool previousFlag = message->m_bSendPlaylists;
	message->m_bIsExtendedClient = true;
	const ScopeGuard restoreFlag([message, previousFlag] { message->m_bSendPlaylists = previousFlag; });
	return message->NET_SignonState::WriteToBuffer(buffer);
})
// block bad netmessages
// Servers can literally request a screenshot from any client, yeah no
// clang-format off
DECLARE_HOOK(CLC_Screenshot::WriteToBuffer, engine.dll + 0x22AF20,
([](auto& hook, void* thisptr, void* buffer) -> bool // 48 89 5C 24 ? 57 48 83 EC 20 8B 42 10
// clang-format on
{
	if (g_pVanillaCompatibility->GetVanillaCompatibility())
		return hook.Original(thisptr, buffer);
	return false;
}))

// clang-format off
DECLARE_HOOK(CLC_Screenshot::ReadFromBuffer, engine.dll + 0x221F00,
([](auto& hook, void* thisptr, void* buffer) -> bool // 48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC 20 48 8B DA 48 8B 52 38
// clang-format on
{
	if (g_pVanillaCompatibility->GetVanillaCompatibility())
		return hook.Original(thisptr, buffer);
	return false;
}))

// This is unused ingame and a big client=>server=>client exploit vector
// clang-format off
DECLARE_HOOK(Base_CmdKeyValues::ReadFromBuffer, engine.dll + 0x220040,
([](auto& hook, void* thisptr, void* buffer) -> bool // 40 55 48 81 EC ? ? ? ? 48 8D 6C 24 ? 48 89 5D 70
// clang-format on
{
	NOTE_UNUSED(hook);
	NOTE_UNUSED(thisptr);
	NOTE_UNUSED(buffer);
	return false;
}))
// clang-format off
DECLARE_HOOK(NET_SetConVar::Process, engine.dll + 0x75CF0,
([](auto& hook, NET_SetConVar* msg) -> bool // 48 8B D1 48 8B 49 18 48 8B 01 48 FF 60 10
// clang-format on
{

	constexpr int ENTRY_STR_LEN = MAX_OSPATH;
	bool bIsServerFrame = ThreadInServerFrameThread();

	std::string BLOCK_PREFIX =
		std::string {"NET_SetConVar ("} + (bIsServerFrame ? "server" : "client") + "): Blocked dangerous/invalid msg: ";

	if (bIsServerFrame)
	{
		constexpr int SETCONVAR_SANITY_AMOUNT_LIMIT = 69;
		if (msg->m_ConVars.Count() < 1 || msg->m_ConVars.Count() > SETCONVAR_SANITY_AMOUNT_LIMIT)
		{
			return BLOCKED_INFO("Invalid m_ConVars_count (" << msg->m_ConVars.Count() << ")");
		}
	}

	for (int i = 0; i < msg->m_ConVars.Count(); i++)
	{
		auto entry = msg->m_ConVars.Base() + i;

		// Safety check for memory access
		if (CMemory(entry).IsMemoryReadable(sizeof(*entry)))
		{
			// Find null terminators
			bool nameValid = false, valValid = false;
			for (int i = 0; i < ENTRY_STR_LEN; i++)
			{
				if (!entry->name[i])
					nameValid = true;
				if (!entry->value[i])
					valValid = true;
			}

			if (!nameValid || !valValid)
				return BLOCKED_INFO("Missing null terminators");

			// we only need to check if these cvars are valid on client as it will set actual cvars there
			// on server this won't set any actual convars, only keyvalues in the player, which doesn't have really any potential for dumb
			// stuff
			if (!bIsServerFrame)
			{
				ConVar* pVar = g_pCVar->FindVar(entry->name);
				if (pVar)
				{
					const char* canonicalName = pVar->GetName();
					memcpy(entry->name, canonicalName, strlen(canonicalName) + 1); // Force name to match case

					if (!pVar->IsFlagSet(FCVAR_REPLICATED))
					{
						spdlog::warn(
							"Blocking replication of remote cvar {} from server (server's var has flag REPLICATED, while ours does not)",
							entry->name);

						// don't block, as non-malicious servers might send bad cvars, and we still want those clients to be able to
						// connect
						memset(entry->name, 0, ENTRY_STR_LEN);
						memset(entry->value, 0, ENTRY_STR_LEN);
					}
				}
			}
		}
		else
		{
			return BLOCKED_INFO("Unreadable memory at " << (void*)entry); // Not risking that one, they all gotta be readable
		}
	}

	return hook.Original(msg);
}))
static bool(__fastcall* s_NET_BufferToBufferCompress)(void* pDest, __int64* pDestLen, const void* pSrc, int srcLen) = nullptr;

struct VSVC_PlaylistsLayout
{
    char pad0[0x20];
    bool hasCompressedData; // +0x20
    char pad1[0x3];         // +0x21
    int dataBytes;          // +0x24
    void* dataPtr;          // +0x28
    int uncompressedBytes;  // +0x30
};

static_assert(offsetof(VSVC_PlaylistsLayout, hasCompressedData) == 0x20);
static_assert(offsetof(VSVC_PlaylistsLayout, dataBytes) == 0x24);
static_assert(offsetof(VSVC_PlaylistsLayout, dataPtr) == 0x28);
static_assert(offsetof(VSVC_PlaylistsLayout, uncompressedBytes) == 0x30);

DECLARE_HOOK(SVC_Playlists::WriteToBuffer, engine.dll + 0x22BBD0, [](auto& hook, __int64 msg, bf_write* buffer) -> bool
{
    static thread_local std::vector<unsigned char> s_PlaylistsCompressedTls;

    void* msgPtr = reinterpret_cast<void*>(msg);
    auto* msgLayout = reinterpret_cast<VSVC_PlaylistsLayout*>(msgPtr);
    bool hasCompressedData = msgLayout->hasCompressedData;
    int dataBytes = msgLayout->dataBytes;
    void* dataPtr = msgLayout->dataPtr;
    int uncompressedBytes = msgLayout->uncompressedBytes;
	static ConVar* Cvar_sv_compress_playlists = g_pCVar->FindVar("sv_compressPlaylists");
    const bool canTryCompress =
        !hasCompressedData && dataPtr && dataBytes > 0 && s_NET_BufferToBufferCompress && Cvar_sv_compress_playlists->GetBool();

    bool usedTempCompression = false;
    unsigned char oldCompressedFlag = msgLayout->hasCompressedData ? 1 : 0;
    int oldDataBytes = dataBytes;
    void* oldDataPtr = dataPtr;
    int oldUncompressedBytes = uncompressedBytes;

    if (canTryCompress)
    {
        const size_t neededBytes = static_cast<size_t>(oldDataBytes) + 0x4000;
        if (s_PlaylistsCompressedTls.size() < neededBytes)
            s_PlaylistsCompressedTls.resize(neededBytes);

        __int64 compressedBytes = static_cast<__int64>(s_PlaylistsCompressedTls.size());
        if (s_NET_BufferToBufferCompress(s_PlaylistsCompressedTls.data(), &compressedBytes, oldDataPtr, oldDataBytes) && compressedBytes > 0 &&
            compressedBytes < oldDataBytes)
        {
            msgLayout->hasCompressedData = true;
            msgLayout->dataBytes = static_cast<int>(compressedBytes);
            msgLayout->dataPtr = s_PlaylistsCompressedTls.data();
            msgLayout->uncompressedBytes = oldDataBytes;
            usedTempCompression = true;

            hasCompressedData = true;
            dataBytes = static_cast<int>(compressedBytes);
            dataPtr = s_PlaylistsCompressedTls.data();
            uncompressedBytes = oldDataBytes;
        }
    }

    int bitsBefore = buffer->GetNumBitsWritten();
    int bitsLeftBefore = buffer->GetNumBitsLeft();
    bool overflowBefore = buffer->IsOverflowed();

    bool result = hook.Original(msg, buffer);

    if (usedTempCompression)
    {
        msgLayout->hasCompressedData = (oldCompressedFlag != 0);
        msgLayout->dataBytes = oldDataBytes;
        msgLayout->dataPtr = oldDataPtr;
        msgLayout->uncompressedBytes = oldUncompressedBytes;
    }

    if (!result)
    {
        int bitsAfter = buffer->GetNumBitsWritten();
        bool overflowAfter = buffer->IsOverflowed();

        spdlog::error("  !!! VSVC_Playlists::WriteToBuffer FAILED");
        spdlog::error("      msg=0x{:X} vtbl=0x{:X} compressed={} dataBytes={} uncompressedBytes={} dataPtr=0x{:X}",
                      reinterpret_cast<uintptr_t>(msgPtr), msgPtr ? reinterpret_cast<uintptr_t>(*reinterpret_cast<void**>(msgPtr)) : 0ull,
                      hasCompressedData, dataBytes, uncompressedBytes, reinterpret_cast<uintptr_t>(dataPtr));
        spdlog::error("      buffer: beforeBits={} bitsLeftBefore={} overflowBefore={}, afterBits={} overflowAfter={}", bitsBefore, bitsLeftBefore,
                      overflowBefore, bitsAfter, overflowAfter);
    }
    else if (usedTempCompression)
    {
        spdlog::info("  SVC_Playlists compressed for send: {} -> {} bytes", oldDataBytes, dataBytes);
    }

    return result;
})

ON_DLL_LOAD("engine.dll", NetMessages, [](CModule)
{
    DISPATCH_HOOK(NetMessagesHooks, NET_SignonState::ReadFromBuffer)
    DISPATCH_HOOK(NetMessagesHooks, NET_SignonState::WriteToBuffer)
})

ON_DLL_LOAD_RELIESON("engine.dll", NetworkMessageSecurity, ServerLimits, [](CModule)
{
    DISPATCH_HOOK(NetMessagesHooks, CLC_Screenshot::WriteToBuffer)
    DISPATCH_HOOK(NetMessagesHooks, CLC_Screenshot::ReadFromBuffer)
    DISPATCH_HOOK(NetMessagesHooks, Base_CmdKeyValues::ReadFromBuffer)
    DISPATCH_HOOK(NetMessagesHooks, NET_SetConVar::Process)
})

ON_DLL_LOAD_RELIESON("engine.dll", PlaylistMessageCompression, ConCommand, [](CModule module)
{
    s_NET_BufferToBufferCompress = module.Offset(0x218F50).RCast<decltype(s_NET_BufferToBufferCompress)>();
    DISPATCH_HOOK(NetMessagesHooks, SVC_Playlists::WriteToBuffer)
})
