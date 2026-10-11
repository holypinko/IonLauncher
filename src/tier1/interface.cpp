#include "tier1/interface.h"


CMemory Sys_GetFactoryPtr(const std::string& svModuleName, const std::string& svFactoryName)
{
	HMODULE hModule = GetModuleHandleA(svModuleName.c_str());

	if (!hModule)
	{
		spdlog::error("Failed to get module handle of '{}'!", svModuleName.c_str());
		exit(EXIT_FAILURE);
	}

	CreateInterfaceFn fnCreateInterface = reinterpret_cast<CreateInterfaceFn>(GetProcAddress(hModule, CREATEINTERFACE_PROCNAME));

	return fnCreateInterface(svFactoryName.c_str(), NULL);
}
