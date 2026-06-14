#include "CoreModule.h"
#include "Thread/Threading.h"
#include "Logging/LoggingSystem.h"
#include "Thread/ThreadPool.h"

//-------------------------------------------------------------------------

namespace SE
{
    //-------------------------------------------------------------------------
    bool CoreModule::Initialize()
    {
		Platform::Init();
    	Log::System::Initialize();
		Threading::SetMainThreadID(Platform::GetCurrentThreadID());
		Threading::Init();
		CoreTypeRegistry::Initialize();

        return true;
    }

    void CoreModule::Shutdown()
    {
		CoreTypeRegistry::Shutdown();
		Threading::Shutdown();
		Log::System::Shutdown();
		Platform::Exit();
    }
}