#include "EngineContext.h"
#include "Runtime/Core/Types/Strings/String.h"

namespace SE
{
	String EngineContext::StartupFolder;
	String EngineContext::TemporaryFolder;
	String EngineContext::ProjectFolder;
	String EngineContext::ProductLocalFolder;
	String EngineContext::BinariesFolder;
#if SE_EDITOR
	String EngineContext::ProjectCacheFolder;
	String EngineContext::ProjectSourceFolder;
#endif
	String EngineContext::ProjectContentFolder;

	String EngineContext::ProductName;

	bool EngineContext::FatalErrorOccurred;
	bool EngineContext::IsRequestingExit;
	int32 EngineContext::ExitCode;
}