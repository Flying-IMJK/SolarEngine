#pragma once

#include "ResourceMessages.h"
#include "ResourceCompilationRequest.h"

#include "Runtime/Resource/ResourceProvider.h"
#include "Core/Tools/Timers.h"
#include "Core/Thread/Threading.h"
#include "Core/Thread/Task.h"
#include "Core/Platform/FileSystemWatcher.h"

#include "Editor/API.h"
#include "Editor/Resource/ResourceCompiler.h"
#include "ResourceServerContext.h"

//-------------------------------------------------------------------------

#ifdef SE_DEVELOPMENT
namespace SGE
{
    class ResourceGlobalSettings;
    struct ModuleContext;
}


namespace SGE::Editor
{
    class CompilationTask;
    class PackagingTask;
	class ServerTask;
	class CompiledResourceDatabase;

    //-------------------------------------------------------------------------

    class SE_API_EDITOR ResourceServerProvider final : public ResourceProvider
    {
    public:
        struct BusyState
        {
            int32     m_completedRequests = 0;
            int32     m_totalRequests = 0;
            bool      m_isBusy = false;
        };

    public:
        ResourceServerProvider(ResourceGlobalSettings const &settings) : ResourceProvider(settings) {}

        virtual bool IsReady() const override final;

		virtual bool Initialize() override final;
		virtual void Shutdown() override final;
		virtual void Update() override final;

		virtual void RequestRawResource(ResourceRequest *pRequest) override;
		virtual void CancelRequest(ResourceRequest *pRequest) override;

		void CompileResource(ResID const& resourceID, bool forceRecompile = true);
		void PackageResource(ResID const& resourceID );

		CompiledResourceDatabase& GetCompiledDatabase() { return *m_CompiledResourceDatabase; };

    private:
        virtual List<ResID> const &GetExternallyUpdatedResources() const override { return m_externallyUpdatedResources; }

    private:
		CompiledResourceDatabase*			 m_CompiledResourceDatabase;

        List<ResourceRequest*>               m_sentRequests;    // Request that were sent but we're still waiting for a response

        List<ResID> 						 m_externallyUpdatedResources;

		ServerTask*							 m_ServerTask;
		ResourceServerContext				 m_Context;
    };
}
#endif