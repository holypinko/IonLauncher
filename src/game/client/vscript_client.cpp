#include "vscript/languages/squirrel_re/squirrel.h"
#include "engine/client/clientstate.h"
#include "tier1/convar.h"

ADD_SQFUNC("int", NSGetSignonState, "", "Returns the current signon state of the client.", ScriptContext::CLIENT | ScriptContext::UI)
{
	CClientState* client = GetBaseLocalClient();
	if (!client)
	{
		g_pSquirrel[context]->pushinteger(sqvm, -1);
		return SQRESULT_NOTNULL;
	}

	g_pSquirrel[context]->pushinteger(sqvm, static_cast<int>(client->m_nSignonState));
	return SQRESULT_NOTNULL;
}

void ConCommand_ns_script_servertoclientstringcommand(const CCommand& arg)
{
	if (g_pSquirrel[ScriptContext::CLIENT]->m_pSQVM)
		g_pSquirrel[ScriptContext::CLIENT]->Call("NSClientCodeCallback_RecievedServerToClientStringCommand", arg.ArgS());
}

ON_DLL_LOAD_CLIENT_RELIESON("client.dll", ScriptServerToClientStringCommand, ClientSquirrel, [](CModule module)
{
	RegisterConCommand(
		"ns_script_servertoclientstringcommand",
		ConCommand_ns_script_servertoclientstringcommand,
		"",
		FCVAR_CLIENTDLL | FCVAR_SERVER_CAN_EXECUTE);
})
