#include "engine/server/sv_client.h"
#include "game/server/player.h"
#include "vscript/languages/squirrel_re/squirrel.h"

ADD_SQFUNC("void", NSSetVoiceCommsBanned, "entity ent, bool value", "", ScriptContext::SERVER)
{
	auto pPlayer = g_pSquirrel[context]->template getentity<CPlayer>(sqvm, 1);

	bool value = g_pSquirrel[context]->getbool(sqvm, 2);
	if (!pPlayer)
	{
		spdlog::error("bad ent");
		g_pSquirrel[context]->raiseerror(sqvm, "bad ent");
		return SQRESULT_ERROR;
	}
	int32_t client_idx = (pPlayer->m_nPlayerIndex) - 1;
	if (client_idx < 0 || client_idx > 32)
	{
		spdlog::error("bad client index: {}",client_idx);
		g_pSquirrel[context]->raiseerror(sqvm, "bad ent index");
		return SQRESULT_ERROR;
	}
	auto client = &g_pClientArray[client_idx];
	if (!client)
	{
		spdlog::error("bad client{}", client_idx);
		g_pSquirrel[context]->raiseerror(sqvm, "bad client");
		return SQRESULT_ERROR;
	}
	auto extend = client->GetClientExtended();
	if (!extend)
	{
		spdlog::error("bad client{}", client_idx);
		g_pSquirrel[context]->raiseerror(sqvm, "bad extend");
		return SQRESULT_ERROR;
	}
	extend->SetClientIsCommsBanned(value);
	return SQRESULT_NOTNULL;
}


ADD_SQFUNC("bool", NSIsVoiceCommsBanned, "entity ent", "", ScriptContext::SERVER)
{
	auto pPlayer = g_pSquirrel[context]->template getentity<CPlayer>(sqvm, 1);
	if (!pPlayer)
	{
		spdlog::error("bad ent");
		g_pSquirrel[context]->raiseerror(sqvm, "bad ent");
		return SQRESULT_ERROR;
	}
	int32_t client_idx = (pPlayer->m_nPlayerIndex) - 1;
	if (client_idx < 0 || client_idx > 32)
	{
		spdlog::error("bad client index: {}", client_idx);
		g_pSquirrel[context]->raiseerror(sqvm, "bad ent index");
		return SQRESULT_ERROR;
	}
	auto client = &g_pClientArray[client_idx];
	if (!client)
	{
		spdlog::error("bad client{}", client_idx);
		g_pSquirrel[context]->raiseerror(sqvm, "bad client");
		return SQRESULT_ERROR;
	}
	auto extend = client->GetClientExtended();
	if (!extend)
	{
		spdlog::error("bad client{}", client_idx);
		g_pSquirrel[context]->raiseerror(sqvm, "bad extend");
		return SQRESULT_ERROR;
	}
	g_pSquirrel[context]->pushbool(sqvm, extend->IsClientCommsBanned());
	return SQRESULT_NOTNULL;
}



