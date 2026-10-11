#include "dedicated/dedicated.h"
#include "masterserver/masterserver.h"
#include "vscript/languages/squirrel_re/squirrel.h"

ADD_SQFUNC("bool", NSIsWritingPlayerPersistence, "", "", ScriptContext::SERVER)
{
    g_pSquirrel[context]->pushbool(sqvm, g_pMasterServerManager->m_bSavingPersistentData);
    return SQRESULT_NOTNULL;
}

ADD_SQFUNC("bool", NSIsDedicated, "", "", ScriptContext::SERVER)
{
    g_pSquirrel[context]->pushbool(sqvm, IsDedicatedServer());
    return SQRESULT_NOTNULL;
}

