ON_DLL_LOAD_RELIESON("server.dll", ServerAntiTamper, ConVar, ([](CModule module)
{
	// Dumb ANTITAMPER patches (they negatively impact performance and security)
	constexpr const char* ANTITAMPER_EXPORTS[] = {
		"ANTITAMPER_SPOTCHECK_CODEMARKER",
		"ANTITAMPER_TESTVALUE_CODEMARKER",
		"ANTITAMPER_TRIGGER_CODEMARKER",
	};

	// Prevent these from actually doing anything
	for (auto exportName : ANTITAMPER_EXPORTS)
	{
		CMemory exportAddr = module.GetExportedFunction(exportName);
		if (exportAddr)
		{
			// Just return, none of them have any args or are userpurge
			exportAddr.Patch("C3");
			spdlog::info("Patched AntiTamper function export \"{}\"", exportName);
		}
	}
}))
