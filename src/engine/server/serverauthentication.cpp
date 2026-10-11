#include "engine/server/serverauthentication.h"
#include "engine/server/serverauthentication_internal.h"
#include "engine/server/server_limits.h"
#include "tier1/cvar.h"
#include "masterserver/masterserver.h"
#include "engine/server/serverpresence.h"
#include "engine/host_state.h"
#include "engine/server/bansystem.h"
#include "tier1/convar.h"
#include "dedicated/dedicated.h"
#include "config/profile.h"
#include "tier0/commandline.h"
#include "engine/server/sv_client.h"
#include "engine/client/clientstate.h"
#include "engine/server/server.h"
#include "engine/server/sv_main.h"

#include <fstream>
#include <filesystem>
#include <string>
#include <thread>

// global vars
ServerAuthenticationManager* g_pServerAuthentication;

void ServerAuthenticationManager::AddRemotePlayer(std::string token, uint64_t uid, std::string username, std::string pdata)
{
	std::string uidS = std::to_string(uid);

	RemoteAuthData newAuthData {};
	strncpy_s(newAuthData.uid, sizeof(newAuthData.uid), uidS.c_str(), uidS.length());
	strncpy_s(newAuthData.username, sizeof(newAuthData.username), username.c_str(), username.length());
	newAuthData.pdata = new char[pdata.length()];
	newAuthData.pdataSize = pdata.length();
	memcpy(newAuthData.pdata, pdata.c_str(), newAuthData.pdataSize);

	std::lock_guard<std::mutex> guard(m_AuthDataMutex);
	m_RemoteAuthenticationData[token] = newAuthData;
}

void ServerAuthenticationManager::AddPlayer(CClient* pPlayer, const char* pToken)
{
	PlayerAuthenticationData additionalData;

	if (!pToken)
	{
		additionalData.pdataSize = PERSISTENCE_MAX_SIZE;
		additionalData.usingLocalPdata = pPlayer->m_iPersistenceReady == ePersistenceReady::READY_INSECURE;
		m_PlayerAuthenticationData.insert(std::make_pair(pPlayer, additionalData));
		return;
	}

	auto remoteAuthData = m_RemoteAuthenticationData.find(pToken);
	if (remoteAuthData != m_RemoteAuthenticationData.end())
		additionalData.pdataSize = remoteAuthData->second.pdataSize;
	else
		additionalData.pdataSize = PERSISTENCE_MAX_SIZE;

	additionalData.usingLocalPdata = pPlayer->m_iPersistenceReady == ePersistenceReady::READY_INSECURE;

	m_PlayerAuthenticationData.insert(std::make_pair(pPlayer, additionalData));
}

void ServerAuthenticationManager::RemovePlayer(CClient* pPlayer)
{
	if (m_PlayerAuthenticationData.count(pPlayer))
		m_PlayerAuthenticationData.erase(pPlayer);
}

bool ServerAuthenticationManager::VerifyPlayerName(const char* pAuthToken, const char* pName, char pOutVerifiedName[64])
{
	std::lock_guard<std::mutex> guard(m_AuthDataMutex);

	// always use name from masterserver if available
	// use of strncpy_s here should verify that this is always nullterminated within valid buffer size
	auto authData = m_RemoteAuthenticationData.find(pAuthToken);
	if (authData != m_RemoteAuthenticationData.end() && *authData->second.username)
		strncpy_s(pOutVerifiedName, 64, authData->second.username, 63);
	else
		strncpy_s(pOutVerifiedName, 64, pName, 63);

	// now, check that whatever name we have is actually valid
	// first, make sure it's >1 char
	if (!*pOutVerifiedName)
		return false;

	// next, make sure it's within a valid range of ascii characters
	for (int i = 0; pOutVerifiedName[i]; i++)
	{
		if (pOutVerifiedName[i] < 32 || pOutVerifiedName[i] > 126)
			return false;
	}

	return true;
}

bool ServerAuthenticationManager::IsDuplicateAccount(CClient* pPlayer, const char* pPlayerUid)
{
	if (m_bAllowDuplicateAccounts)
		return false;

	bool bHasUidPlayer = false;
	for (int i = 0; i < g_pGlobals->maxClients; i++)
		if (&g_pClientArray[i] != pPlayer && !strcmp(pPlayerUid, g_pClientArray[i].m_szPlatformID))
			return true;

	return false;
}

bool ServerAuthenticationManager::CheckAuthentication(CClient* pPlayer, uint64_t iUid, char* pAuthToken)
{
	std::string sUid = std::to_string(iUid);

	// check whether this player's authentication is valid but don't actually write anything to the player, we'll do that later
	// if we don't need auth this is valid
	if (Cvar_ns_auth_allow_insecure->GetBool())
		return true;

	// local server that doesn't need auth (probably sp) and local player
	if (m_bStartingLocalSPGame && !strcmp(sUid.c_str(), g_pLocalPlayerUserID))
		return true;

	// don't allow duplicate accounts
	if (IsDuplicateAccount(pPlayer, sUid.c_str()))
		return false;

	std::lock_guard<std::mutex> guard(m_AuthDataMutex);
	auto authData = m_RemoteAuthenticationData.find(pAuthToken);
	if (authData != m_RemoteAuthenticationData.end() && !strcmp(sUid.c_str(), authData->second.uid))
		return true;

	return false;
}

void ServerAuthenticationManager::AuthenticatePlayer(CClient* pPlayer, uint64_t iUid, char* pAuthToken)
{
	// for bot players, generate a new uid
	if (pPlayer->m_bFakePlayer)
		iUid = 0; // is this a good way of doing things :clueless:
	std::string sUid = std::to_string(iUid);

	// copy uuid
	strcpy(pPlayer->m_szPlatformID, sUid.c_str());

	std::lock_guard<std::mutex> guard(m_AuthDataMutex);
	if (!pAuthToken)
	{
		pPlayer->m_iPersistenceReady = ePersistenceReady::READY_INSECURE;
		return;
	}
	auto authData = m_RemoteAuthenticationData.find(pAuthToken);
	if (authData != m_RemoteAuthenticationData.end())
	{
		// if we're resetting let script handle the reset with InitPersistentData() on connect
		if (!m_bForceResetLocalPlayerPersistence || strcmp(sUid.c_str(), g_pLocalPlayerUserID))
		{
			// copy pdata into buffer
			memcpy(pPlayer->m_PersistenceBuffer, authData->second.pdata, authData->second.pdataSize);
		}

		// set persistent data as ready
		pPlayer->m_iPersistenceReady = ePersistenceReady::READY_REMOTE;
	}
	// we probably allow insecure at this point, but make sure not to write anyway if not insecure
	else if (Cvar_ns_auth_allow_insecure->GetBool() || pPlayer->m_bFakePlayer)
	{
		// set persistent data as ready
		// note: actual placeholder persistent data is populated in script with InitPersistentData()
		pPlayer->m_iPersistenceReady = ePersistenceReady::READY_INSECURE;
	}
}

bool ServerAuthenticationManager::RemovePlayerAuthData(CClient* pPlayer)
{
	if (!Cvar_ns_erase_auth_info->GetBool()) // keep auth data forever
		return false;

	// hack for special case where we're on a local server, so we erase our own newly created auth data on disconnect
	if (m_bNeedLocalAuthForNewgame && !strcmp(pPlayer->m_szPlatformID, g_pLocalPlayerUserID))
		return false;

	// we don't have our auth token at this point, so lookup authdata by uid
	for (auto& auth : m_RemoteAuthenticationData)
	{
		if (!strcmp(pPlayer->m_szPlatformID, auth.second.uid))
		{
			// pretty sure this is fine, since we don't iterate after the erase
			// i think if we iterated after it'd be undefined behaviour tho
			std::lock_guard<std::mutex> guard(m_AuthDataMutex);

			delete[] auth.second.pdata;
			m_RemoteAuthenticationData.erase(auth.first);
			return true;
		}
	}

	return false;
}

void ServerAuthenticationManager::WritePersistentData(CClient* pPlayer)
{
	if (pPlayer->m_iPersistenceReady == ePersistenceReady::READY_REMOTE)
	{
		g_pMasterServerManager->WritePlayerPersistentData(
			pPlayer->m_szPlatformID, (const char*)pPlayer->m_PersistenceBuffer, m_PlayerAuthenticationData[pPlayer].pdataSize);
	}
	else if (Cvar_ns_auth_allow_insecure_write->GetBool())
	{
		// todo: write pdata to disk here
	}
}


void ConCommand_ns_resetpersistence(const CCommand& args)
{
	NOTE_UNUSED(args);
	if (g_pServer && g_pServer->GetState() == ss_active)
	{
		spdlog::error("ns_resetpersistence must be entered from the main menu");
		return;
	}

	spdlog::info("resetting persistence on next lobby load...");
	g_pServerAuthentication->m_bForceResetLocalPlayerPersistence = true;
}


ON_DLL_LOAD_RELIESON("engine.dll", ServerAuthentication, (ConCommand, ConVar), [](CModule module)
{
	DispatchServerAuthenticationHooks();
	DispatchClientAuthenticationHooks();
	g_pServerAuthentication = new ServerAuthenticationManager;

	g_pServerAuthentication->Cvar_ns_erase_auth_info =
		new ConVar("ns_erase_auth_info", "1", FCVAR_GAMEDLL, "Whether auth info should be erased from this server on disconnect or crash");
	g_pServerAuthentication->Cvar_ns_auth_allow_insecure =
		new ConVar("ns_auth_allow_insecure", "0", FCVAR_GAMEDLL, "Whether this server will allow unauthenicated players to connect");
	g_pServerAuthentication->Cvar_ns_auth_allow_insecure_write = new ConVar(
		"ns_auth_allow_insecure_write",
		"0",
		FCVAR_GAMEDLL,
		"Whether the pdata of unauthenticated clients will be written to disk when changed");

	RegisterConCommand(
		"ns_resetpersistence", ConCommand_ns_resetpersistence, "resets your pdata when you next enter the lobby", FCVAR_NONE);

	ConfigureServerAuthentication(module);
	ConfigureClientAuthentication(module);


	if (CommandLine()->CheckParm("-allowdupeaccounts"))
	{
		AllowDuplicateServerAccounts(module);

		g_pServerAuthentication->m_bAllowDuplicateAccounts = true;
	}
})
