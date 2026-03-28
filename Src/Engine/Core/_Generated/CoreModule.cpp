#include "CoreModule.h"
#include "Core/Thread/Threading.h"
#include "Core/Logging/LoggingSystem.h"
#include "Core/Thread/ThreadPool.h"

//-------------------------------------------------------------------------

namespace SE
{
    //-------------------------------------------------------------------------
    bool CoreModule::Initialize()
    {
		Platform::Init();
		Threading::SetMainThreadID(Platform::GetCurrentThreadID());
		Threading::Init();
		Log::System::Initialize();
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