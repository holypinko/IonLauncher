#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "dedicated/dedicated.h"
#include "dedicated/dedicatedlogtoclient.h"
#include "engine/client/clientstate.h"
#include "engine/server/serverauthentication.h"
#include "engine/server/sv_client.h"
#include "engine/server/sv_main.h"
#include "game/server/player.h"
#include "game/shared/usercmd.h"
#include "tier0/hooks.h"
#include "tier1/convar.h"
#include "tier1/cvar.h"
#include "vscript/languages/squirrel_re/squirrel.h"

CPlayer*(__fastcall* UTIL_PlayerByIndex)(int playerIndex);
static CBaseEntity* (*s_ServerGetEntityByIndex)(int index);

DECLARE_MODULE(PlayerHooks)

uintptr_t s_ServerBase;

ConVar* Cvar_ns_enable_weapon_attack_callback;
ConVar* Cvar_ns_skip_historical_hitbox_check;
ConVar* Cvar_ns_allow_post_death_shots;

struct PostDeathWeaponState
{
    std::uint32_t playerHandle = 0;
    std::uint32_t weaponHandle = 0;
    std::uint32_t deathTick = 0;
    float deathTime = 0;
    Vector3D viewOffset;
    bool spawnParity = false;
    bool active = false;
};

struct PostDeathReplayContext
{
    CPlayer* player;
    int observerMode;
    Vector3D viewOffset;
    bool spawnParity;
    float deathTime;
    bool observerChanged = false;
};

std::array<PostDeathWeaponState, 64> s_PostDeathWeapons;
const ConVar* s_PostDeathMaxUnlag;
thread_local CPlayer* s_PostDeathProtectedPlayer;
thread_local PostDeathReplayContext* s_PostDeathReplay;
thread_local bool s_ReachedRayGeometry;

const char* GetWeaponName(int index)
{
    return reinterpret_cast<const char* (*)(int)>(s_ServerBase + 0x691300)(index);
}

void* GetWeaponOwner(void* weapon)
{
    return reinterpret_cast<void* (*)(void*)>(s_ServerBase + 0xA6A20)(weapon);
}

CBaseEntity* CBaseCombatCharacter::GetActiveWeapon()
{
    return reinterpret_cast<CBaseEntity* (*)(CBaseCombatCharacter*)>(s_ServerBase + 0xE38D0)(this);
}

bool CBaseCombatCharacter::IsWeaponHolstered()
{
    return reinterpret_cast<bool (*)(CBaseCombatCharacter*)>(s_ServerBase + 0xEA760)(this);
}

void CBaseCombatCharacter::HolsterWeapon()
{
    reinterpret_cast<void (*)(CBaseCombatCharacter*)>(s_ServerBase + 0xE9AD0)(this);
}

void CPlayer::ItemPostFrame()
{
    reinterpret_cast<void (*)(CPlayer*)>(s_ServerBase + 0x5D52F0)(this);
}

ADD_SQFUNC("void", NSEarlyWritePlayerPersistenceForLeave, "entity player", "", ScriptContext::SERVER)
{
    const CPlayer* pPlayer = g_pSquirrel[context]->template getentity<CPlayer>(sqvm, 1);
    if (!pPlayer)
    {
        spdlog::warn("NSEarlyWritePlayerPersistenceForLeave got null player");

        g_pSquirrel[context]->pushbool(sqvm, false);
        return SQRESULT_NOTNULL;
    }

    CClient* pClient = &g_pClientArray[pPlayer->m_nPlayerIndex - 1];
    if (g_pServerAuthentication->m_PlayerAuthenticationData.find(pClient) == g_pServerAuthentication->m_PlayerAuthenticationData.end())
    {
        g_pSquirrel[context]->pushbool(sqvm, false);
        return SQRESULT_NOTNULL;
    }

    g_pServerAuthentication->m_PlayerAuthenticationData[pClient].needPersistenceWriteOnLeave = false;
    g_pServerAuthentication->WritePersistentData(pClient);
    return SQRESULT_NULL;
}

ADD_SQFUNC("bool", NSIsPlayerLocalPlayer, "entity player", "", ScriptContext::SERVER)
{
    if (IsDedicatedServer())
    {
        g_pSquirrel[context]->pushbool(sqvm, false);
        return SQRESULT_NOTNULL;
    }
    const CPlayer* pPlayer = g_pSquirrel[ScriptContext::SERVER]->template getentity<CPlayer>(sqvm, 1);
    if (!pPlayer)
    {
        spdlog::warn("NSIsPlayerLocalPlayer got null player");
        g_pSquirrel[context]->pushbool(sqvm, false);
        return SQRESULT_NOTNULL;
    }
    if (!g_pLocalPlayerUserID)
    {
        spdlog::warn("NSIsPlayerLocalPlayer got null uid");
        g_pSquirrel[context]->pushbool(sqvm, false);
        return SQRESULT_NOTNULL;
    }
    CClient* pClient = &g_pClientArray[pPlayer->m_nPlayerIndex - 1];
    g_pSquirrel[context]->pushbool(sqvm, !strcmp(g_pLocalPlayerUserID, pClient->m_szPlatformID));
    return SQRESULT_NOTNULL;
}

ADD_SQFUNC("bool", NSIsPlayerExtendedClient, "entity player", "", ScriptContext::SERVER)
{
    const CPlayer* pPlayer = g_pSquirrel[context]->template getentity<CPlayer>(sqvm, 1);
    const std::uint32_t playerIndex = pPlayer ? pPlayer->m_nPlayerIndex : 0;
    if (playerIndex == 0 || playerIndex > MAX_PLAYERS)
    {
        g_pSquirrel[context]->pushbool(sqvm, false);
        return SQRESULT_NOTNULL;
    }

    const CClientExtended* pExtendedClient = g_pClientArray[playerIndex - 1].GetClientExtended();
    g_pSquirrel[context]->pushbool(sqvm, pExtendedClient && pExtendedClient->IsExtendedClient());
    return SQRESULT_NOTNULL;
}

ADD_SQFUNC("bool", NSDisconnectPlayer, "entity player, string reason", "Disconnects the player from the server with the given reason",
           ScriptContext::SERVER)
{
    const CPlayer* pPlayer = g_pSquirrel[context]->template getentity<CPlayer>(sqvm, 1);
    const char* reason = g_pSquirrel[context]->getstring(sqvm, 2);

    if (!pPlayer)
    {
        spdlog::warn("Attempted to call NSDisconnectPlayer() with null player.");

        g_pSquirrel[context]->pushbool(sqvm, false);
        return SQRESULT_NOTNULL;
    }

    // Shouldn't happen but I like sanity checks.
    CClient* pClient = &g_pClientArray[pPlayer->m_nPlayerIndex - 1];
    if (!pClient)
    {
        spdlog::warn("NSDisconnectPlayer(): player entity has null CClient!");

        g_pSquirrel[context]->pushbool(sqvm, false);
        return SQRESULT_NOTNULL;
    }

    if (reason)
        pClient->Disconnect(REP_REMOVE_ONLY, reason);
    else
        pClient->Disconnect(REP_REMOVE_ONLY, "Disconnected by the server.");

    g_pSquirrel[context]->pushbool(sqvm, true);
    return SQRESULT_NOTNULL;
}

ADD_SQFUNC("void", NSSendClientPrint, "entity player, string msg", "Sends a message to the player's console", ScriptContext::SERVER)
{
    const CPlayer* pPlayer = g_pSquirrel[context]->template getentity<CPlayer>(sqvm, 1);
    const char* msg = g_pSquirrel[context]->getstring(sqvm, 2);

    if (!pPlayer)
    {
        spdlog::warn("NSSendClientPrint(): got null player");
        return SQRESULT_NULL;
    }

    CClient* pClient = &g_pClientArray[pPlayer->m_nPlayerIndex - 1];
    CGameClient__ClientPrintf(pClient, "%s", msg);

    return SQRESULT_NULL;
}

// clang-format off
ADD_SQFUNC("string", GetUserInfoKVString_Internal, "entity player, string key, string defaultValue = \"\"",
	"Gets the string value of a given player's userinfo convar by name", ScriptContext::SERVER)
// clang-format on
{
    const CPlayer* pPlayer = g_pSquirrel[ScriptContext::SERVER]->template getentity<CPlayer>(sqvm, 1);
    if (!pPlayer)
	{
		g_pSquirrel[ScriptContext::SERVER]->raiseerror(sqvm, "player is null");
		return SQRESULT_ERROR;
	}

	const char* pKey = g_pSquirrel[ScriptContext::SERVER]->getstring(sqvm, 2);
	const char* pDefaultValue = g_pSquirrel[ScriptContext::SERVER]->getstring(sqvm, 3);

	const char* pResult = g_pClientArray[pPlayer->m_nPlayerIndex - 1].m_ConVars->GetString(pKey, pDefaultValue);
	g_pSquirrel[ScriptContext::SERVER]->pushstring(sqvm, pResult);
	return SQRESULT_NOTNULL;
}

// clang-format off
ADD_SQFUNC("asset", GetUserInfoKVAsset_Internal, "entity player, string key, asset defaultValue = $\"\"",
	"Gets the asset value of a given player's userinfo convar by name", ScriptContext::SERVER)
// clang-format on
{
	const CPlayer* pPlayer = g_pSquirrel[ScriptContext::SERVER]->template getentity<CPlayer>(sqvm, 1);
	if (!pPlayer)
	{
		g_pSquirrel[ScriptContext::SERVER]->raiseerror(sqvm, "player is null");
		return SQRESULT_ERROR;
	}

	const char* pKey = g_pSquirrel[ScriptContext::SERVER]->getstring(sqvm, 2);
	const char* pDefaultValue;
	g_pSquirrel[ScriptContext::SERVER]->getasset(sqvm, 3, &pDefaultValue);

	const char* pResult = g_pClientArray[pPlayer->m_nPlayerIndex - 1].m_ConVars->GetString(pKey, pDefaultValue);
	g_pSquirrel[ScriptContext::SERVER]->pushasset(sqvm, pResult);
	return SQRESULT_NOTNULL;
}

// clang-format off
ADD_SQFUNC("int", GetUserInfoKVInt_Internal, "entity player, string key, int defaultValue = 0",
	"Gets the int value of a given player's userinfo convar by name", ScriptContext::SERVER)
// clang-format on
{
	const CPlayer* pPlayer = g_pSquirrel[ScriptContext::SERVER]->template getentity<CPlayer>(sqvm, 1);
	if (!pPlayer)
	{
		g_pSquirrel[ScriptContext::SERVER]->raiseerror(sqvm, "player is null");
		return SQRESULT_ERROR;
	}

	const char* pKey = g_pSquirrel[ScriptContext::SERVER]->getstring(sqvm, 2);
	const int iDefaultValue = g_pSquirrel[ScriptContext::SERVER]->getinteger(sqvm, 3);

	const int iResult = g_pClientArray[pPlayer->m_nPlayerIndex - 1].m_ConVars->GetInt(pKey, iDefaultValue);
	g_pSquirrel[ScriptContext::SERVER]->pushinteger(sqvm, iResult);
	return SQRESULT_NOTNULL;
}

// clang-format off
ADD_SQFUNC("float", GetUserInfoKVFloat_Internal, "entity player, string key, float defaultValue = 0",
	"Gets the float value of a given player's userinfo convar by name", ScriptContext::SERVER)
// clang-format on
{
	const CPlayer* pPlayer = g_pSquirrel[ScriptContext::SERVER]->getentity<CPlayer>(sqvm, 1);
	if (!pPlayer)
	{
		g_pSquirrel[ScriptContext::SERVER]->raiseerror(sqvm, "player is null");
		return SQRESULT_ERROR;
	}

	const char* pKey = g_pSquirrel[ScriptContext::SERVER]->getstring(sqvm, 2);
	const float flDefaultValue = g_pSquirrel[ScriptContext::SERVER]->getfloat(sqvm, 3);

	const float flResult = g_pClientArray[pPlayer->m_nPlayerIndex - 1].m_ConVars->GetFloat(pKey, flDefaultValue);
	g_pSquirrel[ScriptContext::SERVER]->pushfloat(sqvm, flResult);
	return SQRESULT_NOTNULL;
}

// clang-format off
ADD_SQFUNC("bool", GetUserInfoKVBool_Internal, "entity player, string key, bool defaultValue = false",
	"Gets the bool value of a given player's userinfo convar by name", ScriptContext::SERVER)
// clang-format on
{
	const CPlayer* pPlayer = g_pSquirrel[ScriptContext::SERVER]->getentity<CPlayer>(sqvm, 1);
	if (!pPlayer)
	{
		g_pSquirrel[ScriptContext::SERVER]->raiseerror(sqvm, "player is null");
		return SQRESULT_ERROR;
	}

	const char* pKey = g_pSquirrel[ScriptContext::SERVER]->getstring(sqvm, 2);
	const bool bDefaultValue = g_pSquirrel[ScriptContext::SERVER]->getbool(sqvm, 3);

	const bool bResult = g_pClientArray[pPlayer->m_nPlayerIndex - 1].m_ConVars->GetInt(pKey, bDefaultValue);
	g_pSquirrel[ScriptContext::SERVER]->pushbool(sqvm, bResult);
	return SQRESULT_NOTNULL;
}

DECLARE_HOOK(CPlayer::CycleObserverTarget, server.dll + 0x57BFD0, [](auto& hook, CPlayer* player, bool reverse) -> bool
{
    static CPlayer* s_pObserverCycleInProgress;

    if (!player || s_pObserverCycleInProgress != player)
    {
        CPlayer* previousPlayer = s_pObserverCycleInProgress;
        s_pObserverCycleInProgress = player;
        const bool result = hook.Original(player, reverse);
        s_pObserverCycleInProgress = previousPlayer;
        return result;
    }

    return true;
})

DECLARE_HOOK(PrimaryAttack, server.dll + 0x6A0220, [](auto& hook, void* weapon, int attackIndex) -> bool
{
    if (!Cvar_ns_enable_weapon_attack_callback->GetBool() || !weapon)
        return hook.Original(weapon, attackIndex);

    void* player = GetWeaponOwner(weapon);
    const int weaponNameIndex = player ? *reinterpret_cast<const int*>(reinterpret_cast<const std::byte*>(weapon) + 0x12D8) : -1;
    const char* weaponName = weaponNameIndex >= 0 ? GetWeaponName(weaponNameIndex) : nullptr;
    if (!weaponName || !*weaponName)
        return hook.Original(weapon, attackIndex);

    bool ret = hook.Original(weapon, attackIndex);
    if (!ret)
        return ret;

    auto* sq = g_pSquirrel[ScriptContext::SERVER];
    if (!sq->m_pSQVM || !sq->m_pSQVM->sqvm)
        return ret;

    weaponName = GetWeaponName(weaponNameIndex);
    auto playerInstance = sq->__sq_createscriptinstance(player);
    auto weaponInstance = sq->__sq_createscriptinstance(weapon);

    if (playerInstance && weaponInstance && weaponName)
        sq->Call("CodeCallback_OnWeaponAttack", playerInstance, weaponInstance, weaponName, 1);

    return ret;
})

DECLARE_HOOK(LagCompensationCalcDistanceSqrToLineSegment, server.dll + 0x6EF440,
             [](auto& Hook, const Vector3D* Point, const Vector3D* SegmentStart, const Vector3D* SegmentEnd, float* OutFraction) -> float
{
    if (reinterpret_cast<uintptr_t>(Hook.ReturnAddress()) - s_ServerBase == 0x5C4D3B)
        s_ReachedRayGeometry = true;

    return Hook.Original(Point, SegmentStart, SegmentEnd, OutFraction);
})

DECLARE_HOOK(LagCompensationShotShapeAllowsEntity, server.dll + 0x5C4A30,
             [](auto& Hook, const void* Manager, CBaseEntity* Candidate, void* TransmitInfo, unsigned int AttackerMask) -> bool
{
    const bool PreviousRayGeometry = s_ReachedRayGeometry;
    s_ReachedRayGeometry = false;

    const bool NativeResult = Hook.Original(Manager, Candidate, TransmitInfo, AttackerMask);
    const bool ReachedRayGeometry = s_ReachedRayGeometry;
    s_ReachedRayGeometry = PreviousRayGeometry;

    return NativeResult || (Cvar_ns_skip_historical_hitbox_check->GetBool() && ReachedRayGeometry && Candidate && Candidate->IsPlayer());
})

DECLARE_HOOK(PostDeathSetObserverMode, server.dll + 0x5B26D0, [](auto& hook, CPlayer* player, int mode)
{
    if (s_PostDeathReplay && s_PostDeathReplay->player == player && !s_PostDeathReplay->observerChanged)
    {
        player->m_iObserverMode = s_PostDeathReplay->observerMode;
        s_PostDeathReplay->observerChanged = true;
    }
	hook.Original(player, mode);
})

DECLARE_HOOK(PostDeathStopObserverMode, server.dll + 0x5B5DA0, [](auto& hook, CPlayer* player)
{
    if (s_PostDeathReplay && s_PostDeathReplay->player == player && !s_PostDeathReplay->observerChanged)
    {
        player->m_iObserverMode = s_PostDeathReplay->observerMode;
        s_PostDeathReplay->observerChanged = true;
    }
	hook.Original(player);
})

DECLARE_HOOK(PostDeathScriptStopObserverMode, server.dll + 0x581640, [](auto& hook, HSQUIRRELVM sqvm) -> SQRESULT
{
    if (s_PostDeathReplay)
    {
        CPlayer* player = nullptr;
        if (!g_pSquirrel[ScriptContext::SERVER]->getthisentity(sqvm, &player))
            return SQRESULT_ERROR;

		if (s_PostDeathReplay && s_PostDeathReplay->player == player && !s_PostDeathReplay->observerChanged)
    	{
    	    player->m_iObserverMode = s_PostDeathReplay->observerMode;
    	    s_PostDeathReplay->observerChanged = true;
    	}
	}
    return hook.Original(sqvm);
})

DECLARE_HOOK(PostDeathHolsterWeapon, server.dll + 0xE9AD0, [](auto& hook, CBaseCombatCharacter* player)
{
    if (player == s_PostDeathProtectedPlayer)
    {
        const uintptr_t caller = reinterpret_cast<uintptr_t>(hook.ReturnAddress()) - s_ServerBase;
        if (caller == 0x58B16A || caller == 0xCDDFA)
            return;
    }
    hook.Original(player);
})

DECLARE_HOOK(PostDeathEyePosition, server.dll + 0x5CF7D0, [](auto& hook, CPlayer* player, Vector3D* result) -> Vector3D*
{
    const PostDeathReplayContext* replay = s_PostDeathReplay;
    if (!replay || replay->player != player || replay->observerChanged || player->m_lifeState == 0 || player->m_iSpawnParity != replay->spawnParity ||
        player->m_flDeathTime != replay->deathTime)
        return hook.Original(player, result);

    Vector3D& viewOffset = *reinterpret_cast<Vector3D*>(reinterpret_cast<std::byte*>(player) + 0x5BC);
    const Vector3D previousViewOffset = viewOffset;
    viewOffset = replay->viewOffset;
    Vector3D* eyePosition = hook.Original(player, result);
    viewOffset = previousViewOffset;
    return eyePosition;
})

DECLARE_HOOK(PostDeathEventKilled, server.dll + 0x58AFC0, [](auto& hook, CPlayer* player, const void* damageInfo)
{
	const auto index = player->m_nPlayerIndex;
    PostDeathWeaponState* state = index > 0 && index <= s_PostDeathWeapons.size() ? &s_PostDeathWeapons[index - 1] : nullptr;

    if (state)
        state->active = false;

    if (!s_PostDeathMaxUnlag)
        s_PostDeathMaxUnlag = g_pCVar->FindVar("sv_maxunlag");
    const float maxUnlag = s_PostDeathMaxUnlag ? s_PostDeathMaxUnlag->GetFloat() : 0.0f;
    CBaseEntity* weapon = nullptr;
    if (state && Cvar_ns_allow_post_death_shots->GetBool() && player->m_lifeState == 0 && g_pGlobals && std::isfinite(maxUnlag) && maxUnlag > 0 &&
        !player->IsWeaponHolstered())
        weapon = player->GetActiveWeapon();

    const std::uint32_t weaponHandle = weapon ? weapon->GetRefEHandle().ToInt() : 0;
    const std::uint32_t deathTick = g_pGlobals ? g_pGlobals->tickcount : 0;
    const Vector3D viewOffset = weapon ? *reinterpret_cast<const Vector3D*>(reinterpret_cast<const std::byte*>(player) + 0x5BC) : Vector3D{};
    CPlayer* previousProtectedPlayer = s_PostDeathProtectedPlayer;
    s_PostDeathProtectedPlayer = weapon ? player : nullptr;
    hook.Original(player, damageInfo);
    s_PostDeathProtectedPlayer = previousProtectedPlayer;

    if (!weapon)
        return;

    CBaseEntity* currentWeapon = player->GetActiveWeapon();
    if (player->m_lifeState == 0 || !currentWeapon || currentWeapon->GetRefEHandle().ToInt() != weaponHandle || player->IsWeaponHolstered() ||
        !std::isfinite(player->m_flDeathTime))
    {
        if (player->m_lifeState != 0 && currentWeapon && currentWeapon->GetRefEHandle().ToInt() == weaponHandle)
            player->HolsterWeapon();
        return;
    }

    state->playerHandle = player->GetRefEHandle().ToInt();
    state->weaponHandle = weaponHandle;
    state->deathTick = deathTick;
    state->deathTime = player->m_flDeathTime;
    state->viewOffset = viewOffset;
    state->spawnParity = player->m_iSpawnParity;
    state->active = true;
})

DECLARE_HOOK(PostDeathPostThink, server.dll + 0x5D7880, [](auto& hook, CPlayer* player)
{
    const bool wasAlive = player->m_lifeState == 0;
    hook.Original(player);
    if (wasAlive)
        return;

	const auto index = player->m_nPlayerIndex;
    PostDeathWeaponState* state = index > 0 && index <= s_PostDeathWeapons.size() ? &s_PostDeathWeapons[index - 1] : nullptr;

    if (!state || !state->active)
        return;

    if (player->m_lifeState == 0 || player->GetRefEHandle().ToInt() != state->playerHandle || player->m_iSpawnParity != state->spawnParity ||
        player->m_flDeathTime != state->deathTime)
    {
        state->active = false;
        return;
    }

    const float maxUnlag = s_PostDeathMaxUnlag->GetFloat();
    if (!Cvar_ns_allow_post_death_shots->GetBool() || !g_pGlobals || !std::isfinite(maxUnlag) || maxUnlag <= 0 ||
        g_pGlobals->tickcount < state->deathTick || (g_pGlobals->tickcount - state->deathTick) * g_pGlobals->interval_per_tick > maxUnlag)
    {
	    state->active = false;
	    CBaseEntity* weapon = player->GetActiveWeapon();
	    if (weapon && weapon->GetRefEHandle().ToInt() == state->weaponHandle)
	        player->HolsterWeapon();

		return;
    }

    CBaseEntity* weapon = player->GetActiveWeapon();
    if (!weapon || weapon->GetRefEHandle().ToInt() != state->weaponHandle || player->IsWeaponHolstered())
    {
        state->active = false;
        return;
    }

    const SV_CUserCmd* command = player->m_pCurrentCommand;
    if (!command || !std::isfinite(command->command_time) || command->command_time > state->deathTime ||
        state->deathTime - command->command_time > maxUnlag)
        return;

    bool& allowDeadOwner = *reinterpret_cast<bool*>(reinterpret_cast<std::byte*>(weapon) + 0x1EC9);
    const bool previousAllowDeadOwner = allowDeadOwner;
    const std::uint32_t weaponHandle = state->weaponHandle;
    PostDeathReplayContext replay =
        (PostDeathReplayContext{player, player->m_iObserverMode, state->viewOffset, state->spawnParity, state->deathTime});
    PostDeathReplayContext* previousReplay = s_PostDeathReplay;
    s_PostDeathReplay = &replay;
    allowDeadOwner = true;
    player->m_iObserverMode = 0;
    player->ItemPostFrame();
    s_PostDeathReplay = previousReplay;
    if (!replay.observerChanged && player->m_lifeState != 0 && player->m_iSpawnParity == replay.spawnParity &&
        player->m_flDeathTime == replay.deathTime && player->m_iObserverMode == 0)
        player->m_iObserverMode = replay.observerMode;

    if (!previousAllowDeadOwner)
    {
        CBaseEntity* retainedWeapon = s_ServerGetEntityByIndex(weaponHandle & 0xFFFF);
        if (retainedWeapon && retainedWeapon->GetRefEHandle().ToInt() == weaponHandle)
            *reinterpret_cast<bool*>(reinterpret_cast<std::byte*>(retainedWeapon) + 0x1EC9) = false;
    }
})

ON_DLL_LOAD_RELIESON("server.dll", CPlayer, (ConVar, CmdBuffer, EngineServerGlobals, EngineSystem), [](CModule module)
{
    Cvar_ns_enable_weapon_attack_callback =
        new ConVar("ns_enable_weapon_attack_callback", "0", FCVAR_GAMEDLL, "Enables script weapon attack callback.");
    Cvar_ns_skip_historical_hitbox_check =
        new ConVar("ns_skip_historical_hitbox_check", "1", FCVAR_GAMEDLL,
                   "Experimental hitreg fix, improves hitreg when enabled but has some minor performance penalty, skips historical hitbox distance check.", true, 0.0f, true,
                   1.0f, nullptr);
    Cvar_ns_allow_post_death_shots =
        new ConVar("ns_allow_post_death_shots", "0", FCVAR_GAMEDLL,
                   "Allow pre-death user commands to finish weapon firing after death. Good if you have people on high ping");

    s_ServerBase = module.GetModuleBase();
    UTIL_PlayerByIndex = module.Offset(0x26AA10).RCast<CPlayer*(__fastcall*)(int)>();
    s_ServerGetEntityByIndex = module.Offset(0xFB820).RCast<decltype(s_ServerGetEntityByIndex)>();

    DISPATCH_MODULE(PlayerHooks);
})
