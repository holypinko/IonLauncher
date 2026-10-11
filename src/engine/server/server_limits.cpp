#include "engine/server/server_limits.h"
#include "common/netmessages.h"
#include "tier0/platform.h"
#include "dedicated/dedicated.h"
#include "engine/server/sv_client.h"
#include "engine/client/clientstate.h"
#include "engine/host_state.h"
#include "engine/server/sv_main.h"
#include "engine/net.h"
#include "mathlib/vector.h"
#include "engine/server/serverauthentication.h"
#include "game/server/player.h"
#include "game/shared/usercmd.h"
#include "tier1/cvar.h"
#include "util/utils.h"

ServerLimitsManager* g_pServerLimits;
ConVar* Cvar_ns_exploitfixes_log;

float (*CEngineServer__GetTimescale)();

// todo: make this work on higher timescales, also possibly disable when sv_cheats is set
void ServerLimitsManager::RunFrame(double flCurrentTime, float flFrameTime)
{
	NOTE_UNUSED(flCurrentTime);
	if (Cvar_sv_antispeedhack_enable->GetBool())
	{
		// for each player, set their usercmd processing budget for the frame to the last frametime for the server
		for (int i = 0; i < g_pGlobals->maxClients; i++)
		{
			CClient* player = &g_pClientArray[i];

			if (m_PlayerLimitData.find(player) != m_PlayerLimitData.end())
			{
				PlayerLimitData* pLimitData = &g_pServerLimits->m_PlayerLimitData[player];
				if (pLimitData->flFrameUserCmdBudget < g_pGlobals->interval_per_tick * Cvar_sv_antispeedhack_maxtickbudget->GetFloat())
				{
					pLimitData->flFrameUserCmdBudget += g_pServerLimits->Cvar_sv_antispeedhack_budgetincreasemultiplier->GetFloat() *
														fmax(flFrameTime, g_pGlobals->frametime * CEngineServer__GetTimescale());
				}
			}
		}
	}
}

void ServerLimitsManager::AddPlayer(CClient* player)
{
	PlayerLimitData limitData;
	limitData.flFrameUserCmdBudget =
		g_pGlobals->interval_per_tick * CEngineServer__GetTimescale() * Cvar_sv_antispeedhack_maxtickbudget->GetFloat();

	m_PlayerLimitData.insert(std::make_pair(player, limitData));
}

void ServerLimitsManager::RemovePlayer(CClient* player)
{
	if (m_PlayerLimitData.find(player) != m_PlayerLimitData.end())
		m_PlayerLimitData.erase(player);
}

bool ServerLimitsManager::CheckStringCommandLimits(CClient* player)
{
	if (CVar_sv_quota_stringcmdspersecond->GetInt() != -1)
	{
		// note: this isn't super perfect, legit clients can trigger it in lobby if they try, mostly good enough tho imo
		if (g_PlatFloatTime() - m_PlayerLimitData[player].lastClientCommandQuotaStart >= 1.0)
		{
			// reset quota
			m_PlayerLimitData[player].lastClientCommandQuotaStart = g_PlatFloatTime();
			m_PlayerLimitData[player].numClientCommandsInQuota = 0;
		}

		m_PlayerLimitData[player].numClientCommandsInQuota++;
		if (m_PlayerLimitData[player].numClientCommandsInQuota > CVar_sv_quota_stringcmdspersecond->GetInt())
		{
			// too many stringcmds, dc player
			return false;
		}
	}

	return true;
}

bool ServerLimitsManager::CheckChatLimits(CClient* player)
{
	if (g_PlatFloatTime() - m_PlayerLimitData[player].lastSayTextLimitStart >= 1.0)
	{
		m_PlayerLimitData[player].lastSayTextLimitStart = g_PlatFloatTime();
		m_PlayerLimitData[player].sayTextLimitCount = 0;
	}

	if (m_PlayerLimitData[player].sayTextLimitCount >= Cvar_sv_max_chat_messages_per_sec->GetInt())
		return false;

	m_PlayerLimitData[player].sayTextLimitCount++;
	return true;
}


bool ServerLimitsManager::CheckConnectionlessPacketLimits(netpacket_t* packet, char packetType)
{
	static const ConVar* Cvar_net_data_block_enabled = g_pCVar->FindVar("net_data_block_enabled");

	if( packetType == A2S_SIGREQ1 )
		return true;

	// don't ratelimit datablock packets as long as datablock is enabled
	if (packet->from.GetType() == NA_IP &&
		(!(packet->data[4] == 'N' && Cvar_net_data_block_enabled->GetBool()) || !Cvar_net_data_block_enabled->GetBool()))
	{
		// bad lookup: optimise later tm
		UnconnectedPlayerLimitData* sendData = nullptr;
		for (UnconnectedPlayerLimitData& foundSendData : g_pServerLimits->m_UnconnectedPlayerLimitData)
		{
			if (!memcmp(packet->from.GetIP(), foundSendData.ip, 16))
			{
				sendData = &foundSendData;
				break;
			}
		}

		if (!sendData)
		{
			sendData = &g_pServerLimits->m_UnconnectedPlayerLimitData.emplace_back();
			memcpy(sendData->ip, packet->from.GetIP(), 16);
		}

		if (g_PlatFloatTime() < sendData->timeoutEnd)
			return false;

		if (g_PlatFloatTime() - sendData->lastQuotaStart >= 1.0)
		{
			sendData->lastQuotaStart = g_PlatFloatTime();
			sendData->packetCount = 0;
		}

		sendData->packetCount++;

		if (sendData->packetCount >= g_pServerLimits->Cvar_sv_querylimit_per_sec->GetInt())
		{
			spdlog::warn(
				"Client went over connectionless ratelimit of {} per sec with packet of type {}",
				g_pServerLimits->Cvar_sv_querylimit_per_sec->GetInt(),
				packet->data[4]);

			// timeout for a minute
			sendData->timeoutEnd = g_PlatFloatTime() + 60.0;
			return false;
		}
	}

	return true;
}





ON_DLL_LOAD_RELIESON("engine.dll", ServerLimits, ConVar, [](CModule module)
{


	g_pServerLimits = new ServerLimitsManager;

	g_pServerLimits->CVar_sv_quota_stringcmdspersecond = new ConVar(
		"sv_quota_stringcmdspersecond",
		"60",
		FCVAR_GAMEDLL,
		"How many string commands per second clients are allowed to submit, 0 to disallow all string commands, -1 to disable");
	g_pServerLimits->Cvar_net_chan_limit_mode =
		new ConVar("net_chan_limit_mode", "0", FCVAR_GAMEDLL, "The mode for netchan processing limits: 0 = warn, 1 = kick");
	g_pServerLimits->Cvar_net_chan_limit_msec_per_sec = new ConVar(
		"net_chan_limit_msec_per_sec",
		"100",
		FCVAR_GAMEDLL,
		"Netchannel processing is limited to so many milliseconds, abort connection if exceeding budget");
	g_pServerLimits->Cvar_sv_querylimit_per_sec = new ConVar("sv_querylimit_per_sec", "15", FCVAR_GAMEDLL, "");
	g_pServerLimits->Cvar_sv_max_chat_messages_per_sec = new ConVar("sv_max_chat_messages_per_sec", "5", FCVAR_GAMEDLL, "");
	g_pServerLimits->Cvar_sv_antispeedhack_enable =
		new ConVar("sv_antispeedhack_enable", "0", FCVAR_NONE, "whether to enable antispeedhack protections");
	g_pServerLimits->Cvar_sv_antispeedhack_maxtickbudget = new ConVar(
		"sv_antispeedhack_maxtickbudget",
		"64",
		FCVAR_GAMEDLL,
		"Maximum number of client-issued usercmd ticks that can be replayed in packet loss conditions");
	g_pServerLimits->Cvar_sv_antispeedhack_budgetincreasemultiplier = new ConVar(
		"sv_antispeedhack_budgetincreasemultiplier",
		"1",
		FCVAR_GAMEDLL,
		"Increase usercmd processing budget by tickinterval * value per tick");
    Cvar_ns_exploitfixes_log =
        new ConVar("ns_exploitfixes_log", "1", FCVAR_GAMEDLL, "Whether to log blocked or corrected network messages");

	CEngineServer__GetTimescale = module.Offset(0x240840).RCast<float (*)()>();
})

