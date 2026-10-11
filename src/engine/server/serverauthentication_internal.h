#pragma once

#include <cstdint>

#include "tier0/module.h"

// Borrowed from the active CServer::ConnectClient call for CClient::Connect.
extern char* pNextPlayerToken;
extern uint64_t iNextPlayerUid;

void DispatchServerAuthenticationHooks();
void DispatchClientAuthenticationHooks();
void ConfigureServerAuthentication(CModule module);
void ConfigureClientAuthentication(CModule module);
void AllowDuplicateServerAccounts(CModule module);
typedef int64_t (*CServer__RejectConnectionType)(void* a1, unsigned int a2, void* a3, const char* a4, ...);
extern CServer__RejectConnectionType CServer__RejectConnection;
