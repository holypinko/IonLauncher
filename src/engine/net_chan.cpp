#include <algorithm>
#include <climits>
#include <cstddef>
#include <cstdint>

#include "tier0/platform.h"
#include "tier0/threadtools.h"
#include "engine/net_chan.h"
#include "tier0/callbacks.h"
#include "tier1/convar.h"
#include "engine/host_state.h"
#include "engine/client/clientstate.h"
#include "engine/server/server_limits.h"

DECLARE_MODULE(NetChanHooks)

using CNetChanCanPacket_t = bool (*)(const CNetChan*);
using CNetChanProcessMessages_t = bool (*)(CNetChan*, bf_read*);
using CNetChanSendNetMsg_t = bool (*)(CNetChan*, INetMessage*, bool, bool);
using CNetChanSendDatagram_t = std::int32_t (*)(CNetChan*, bf_write*);
using CNetChanSetChoked_t = void (*)(CNetChan*);

CNetChanCanPacket_t CNetChan__CanPacket;
CNetChanProcessMessages_t CNetChan__ProcessMessages;
CNetChanSendNetMsg_t CNetChan__SendNetMsg;
CNetChanSendDatagram_t CNetChan__SendDatagram;
CNetChanSetChoked_t CNetChan__SetChoked;

bool CNetChan::CanPacket() const
{
	return CNetChan__CanPacket(this);
}

bool CNetChan::SendNetMsg(INetMessage& message, const bool forceReliable, const bool voice)
{
	return CNetChan__SendNetMsg(this, &message, forceReliable, voice);
}

std::int32_t CNetChan::SendDatagram(bf_write* const datagram)
{
	return CNetChan__SendDatagram(this, datagram);
}

void CNetChan::SetChoked()
{
	CNetChan__SetChoked(this);
}

bool CNetChan::ProcessMessages(bf_read* const message)
{
	return CNetChan__ProcessMessages(this, message);
}

void CNetChan::FreeReceiveList()
{
	m_ReceiveList.blockSize = 0;
	m_ReceiveList.transferSize = 0;

	if (m_ReceiveList.buffer)
	{
		g_pMemAllocSingleton->Free(m_ReceiveList.buffer);
		m_ReceiveList.buffer = nullptr;
	}
}

bool CNetChan::ReadSubChannelData(bf_read& buffer)
{
	uint8_t discardBuffer[FRAGMENT_SIZE];

	m_bInReliableState = true;

	buffer.ReadUBitLong(32);
	buffer.ReadUBitLong(32);
	const uint32_t wireFragment = buffer.ReadUBitLong(10);

	if (wireFragment == 0)
	{
		if (buffer.ReadOneBit() != 0)
		{
			const uint32_t remoteNonce = buffer.ReadUBitLong(32);
			if (remoteNonce != m_nNonceRemote)
			{
				m_nNonceRemote = remoteNonce;
				m_bReceivedRemoteNonce = true;
				m_bInReliableState = false;
				m_nSubInFragments = 0;
				FreeReceiveList();
				m_bPendingRemoteNonceAck = true;
			}
		}
		else
		{
			m_bPendingRemoteNonceAck = false;
		}
	}
	else
	{
		m_bPendingRemoteNonceAck = false;
	}

	int fragmentsToSkip = INT_MAX;
	if (m_bReceivedRemoteNonce)
	{
		fragmentsToSkip = (m_nSubInFragments & 0x3FF) - static_cast<int>(wireFragment);
		if (fragmentsToSkip < 0)
			fragmentsToSkip += 0x400;
	}

	for (;;)
	{
		buffer.ReadUBitLong(32);
		const bool firstFragment = buffer.ReadOneBit() != 0;

		int transferSize = -1;
		int fragmentSize;
		bool isCompressed = false;
		int64_t uncompressedSize = -1;

		if (firstFragment)
		{
			if (fragmentsToSkip == 0 && m_ReceiveList.buffer)
				return false;

			transferSize = static_cast<int>(buffer.ReadUBitLong(19));
			if (transferSize > NET_MAX_PAYLOAD)
				return false;

			fragmentSize = (std::min)(transferSize, FRAGMENT_SIZE);
			isCompressed = buffer.ReadOneBit() != 0;
			if (isCompressed)
				uncompressedSize = buffer.ReadUBitLong(22);
		}
		else
		{
			const bool lastFragment = buffer.ReadOneBit() != 0;
			fragmentSize = lastFragment ? static_cast<std::int32_t>(buffer.ReadUBitLong(10)) : FRAGMENT_SIZE;
		}

		if (fragmentSize > FRAGMENT_SIZE)
			return false;

		if (fragmentsToSkip != 0)
		{
			buffer.ReadBytes(discardBuffer, fragmentSize);
			--fragmentsToSkip;
		}
		else
		{
			dataFragments_t& receive = m_ReceiveList;

			if (firstFragment)
			{
				receive.transferSize = transferSize;
				receive.blockSize = transferSize;
				receive.currentOffset = 0;
				receive.buffer = static_cast<char*>(g_pMemAllocSingleton->Alloc(transferSize));
				receive.isCompressed = isCompressed;
				receive.uncompressedSize = uncompressedSize;
			}

			if (receive.currentOffset + fragmentSize > receive.transferSize)
				return false;

			buffer.ReadBytes(receive.buffer + receive.currentOffset, fragmentSize);
			receive.currentOffset += fragmentSize;
			++m_nSubInFragments;

			if (receive.currentOffset == receive.transferSize)
			{
				if (receive.isCompressed)
				{
					size_t outputSize = static_cast<size_t>(receive.uncompressedSize);
					char* output = static_cast<char*>(g_pMemAllocSingleton->Alloc((outputSize + 3) & ~size_t {3}));

					NET_BufferToBufferDecompress(
						output, &outputSize, receive.buffer, static_cast<size_t>(receive.blockSize));
					g_pMemAllocSingleton->Free(receive.buffer);

					receive.buffer = output;
					receive.blockSize = outputSize;
					receive.isCompressed = false;
				}

				bf_read messages(receive.buffer, static_cast<size_t>(receive.blockSize));
				const bool processed = this->ProcessMessages(&messages);

				if (receive.buffer)
				{
					g_pMemAllocSingleton->Free(receive.buffer);
					receive.buffer = nullptr;
				}

				if (!processed)
					return false;
			}
		}

		if (buffer.ReadOneBit() == 0)
			return true;
	}
}

// clang-format off
DECLARE_HOOK(CNetChan::ReadSubChannelData, engine.dll + 0x211EF0,
([](auto& hook, CNetChan* channel, bf_read* buffer) -> bool
// clang-format on
{
	NOTE_UNUSED(hook);
	return channel->ReadSubChannelData(*buffer);
}))

ON_DLL_LOAD("engine.dll", NetChan, [](CModule module)
{
	CNetChan__CanPacket = module.Offset(0x20F620).RCast<CNetChanCanPacket_t>();
	CNetChan__ProcessMessages = module.Offset(0x2140A0).RCast<CNetChanProcessMessages_t>();
	CNetChan__SendNetMsg = module.Offset(0x213270).RCast<CNetChanSendNetMsg_t>();
	CNetChan__SendDatagram = module.Offset(0x212CD0).RCast<CNetChanSendDatagram_t>();
	CNetChan__SetChoked = module.Offset(0x213760).RCast<CNetChanSetChoked_t>();

	DISPATCH_HOOK(NetChanHooks, CNetChan::ReadSubChannelData)
})
ConVar* Cvar_ns_log_registered_netmessages = nullptr;

std::vector<std::pair<std::string, int>> g_DebugInfoRegisteredNetMessages;

CNetChan__RegisterMessage_t CNetChan__RegisterMessage = nullptr;

// clang-format off
DECLARE_HOOK(CNetChan_RegisterMessage, engine.dll + 0x2129D0, [](auto& hook, CNetChan* thisptr, INetMessage* msg) -> bool
// clang-format on
{
	if (hook.Original(thisptr, msg))
	{
		if(Cvar_ns_log_registered_netmessages && Cvar_ns_log_registered_netmessages->GetBool())
		{
			spdlog::info("Registered netmessage: {} (type: {}, reliable: {}, size: {})",
				msg->GetName(),
				msg->GetType(),
				msg->IsReliable() ? "yes" : "no",
				msg->GetSize());
		}

        auto it = std::find_if(g_DebugInfoRegisteredNetMessages.begin(), g_DebugInfoRegisteredNetMessages.end(),
            [msg](const auto& pair) {
                return pair.first == msg->GetName() && pair.second == msg->GetType();
            });

        if (it == g_DebugInfoRegisteredNetMessages.end())
            g_DebugInfoRegisteredNetMessages.emplace_back(msg->GetName(), msg->GetType());

		return true;
	}

	return false;
})

void ConCommand_ns_dump_registered_netmessages(const CCommand& args)
{
	spdlog::info("Dumping {} registered netmessages:", g_DebugInfoRegisteredNetMessages.size());

	spdlog::info("enum NetMessageType");
	spdlog::info("{");

	int largestIndex = -1;

	for (const auto& [name, type] : g_DebugInfoRegisteredNetMessages)
	{
		spdlog::info("    {} = {},", name, type);
		if (type > largestIndex)
			largestIndex = type;
	}

	spdlog::info("    __NEXT_INDEX__ = {},", largestIndex + 1);

	spdlog::info("};");
}
// clang-format off
DECLARE_HOOK(CNetChan::ProcessMessages, engine.dll + 0x2140A0,
([](auto& hook, void* self, void* buf) -> char
// clang-format on
{
	enum eNetChanLimitMode
	{
		NETCHANLIMIT_WARN,
		NETCHANLIMIT_KICK
	};

	double startTime = g_PlatFloatTime();
	char ret = hook.Original(self, buf);

	// check processing limits, unless we're in a level transition
	if (g_pHostState->m_iCurrentState == HostState_t::HS_RUN && ThreadInServerFrameThread())
	{
		// player that sent the message
		CClient* sender = *(CClient**)((char*)self + 368);

		// if no sender, return
		// relatively certain this is fine?
		if (!sender || !g_pServerLimits->m_PlayerLimitData.count(sender))
			return ret;

		// reset every second
		if (startTime - g_pServerLimits->m_PlayerLimitData[sender].lastNetChanProcessingLimitStart >= 1.0 ||
			g_pServerLimits->m_PlayerLimitData[sender].lastNetChanProcessingLimitStart == -1.0)
		{
			g_pServerLimits->m_PlayerLimitData[sender].lastNetChanProcessingLimitStart = startTime;
			g_pServerLimits->m_PlayerLimitData[sender].netChanProcessingLimitTime = 0.0;
		}
		g_pServerLimits->m_PlayerLimitData[sender].netChanProcessingLimitTime += (g_PlatFloatTime() * 1000) - (startTime * 1000);

		if (g_pServerLimits->m_PlayerLimitData[sender].netChanProcessingLimitTime >=
			g_pServerLimits->Cvar_net_chan_limit_msec_per_sec->GetInt())
		{
			spdlog::warn(
				"Client {} hit netchan processing limit with {}ms of processing time this second (max is {})",
				(char*)sender + 0x16,
				g_pServerLimits->m_PlayerLimitData[sender].netChanProcessingLimitTime,
				g_pServerLimits->Cvar_net_chan_limit_msec_per_sec->GetInt());

			// never kick local player
			if (g_pServerLimits->Cvar_net_chan_limit_mode->GetInt() != NETCHANLIMIT_WARN && strcmp(g_pLocalPlayerUserID, sender->m_szPlatformID))
			{
				sender->Disconnect(REP_REMOVE_ONLY, "Exceeded net channel processing limit");
				return false;
			}
		}
	}

	return ret;
}))

bool CNetChan::RegisterMessage(INetMessage* message)
{
    return CNetChan__RegisterMessage(this, message);
}

ON_DLL_LOAD_RELIESON("engine.dll", NetChanRegistration, ConVar, [](CModule module)
{
    DISPATCH_HOOK(NetChanHooks, CNetChan_RegisterMessage)
    RegisterConCommand("ns_dump_registered_netmessages", ConCommand_ns_dump_registered_netmessages,
                       "Dumps registered netmessages", FCVAR_NONE);
    Cvar_ns_log_registered_netmessages = new ConVar("ns_log_registered_netmessages", "0", FCVAR_NONE,
                                                   "Logs all registered netmessages to console on registration.");
    CNetChan__RegisterMessage = module.Offset(0x2129D0).RCast<CNetChan__RegisterMessage_t>();
})

ON_DLL_LOAD_RELIESON("engine.dll", NetChanProcessingLimits, ServerLimits, [](CModule)
{
    DISPATCH_HOOK(NetChanHooks, CNetChan::ProcessMessages)
})
