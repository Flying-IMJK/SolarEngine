#include "ResourceServerProvider.h"
#include "ResourceCompilerSystem.h"

#include "Runtime/_Module/RuntimeModule.h"
#include "Runtime/Resource/ResourceRequest.h"
#include "Runtime/Settings/GlobalSettings_Resource.h"
#include "Runtime/EngineContext.h"

#include "Core/Serialization/BinarySerialization.h"
#include "Core/Profiler/Profiler.h"
#include "Core/Thread/ThreadPool.h"
#include "Core/Types/Collections/ListExtensions.h"
#include "Core/Types/Collections/ConcurrentQueue.h"


//-------------------------------------------------------------------------
namespace SGE::Editor
{
	enum class PackagingStage
	{
		None, // Not Packaging
		Preparing,
		Packaging,
		Complete
	};

	class CompilationTask final : public Threading::ThreadPoolTask
	{

	public:

		CompilationTask(ResourceServerContext const& context, ResCompilationRequest* pRequest)
			: m_context( context )
			, m_pRequest( pRequest )
		{

		}

		inline ResCompilationRequest* GetRequest() const { return m_pRequest; }

	private:
		virtual bool Run() override
		{
			// If we are not exiting the application and the request needs to be processed
			// Note: we enqueue failed requests as well just to have a uniform code flow
			if ( !m_context.isExiting && !m_pRequest->IsComplete() )
			{
				ENGINE_ASSERT( !m_pRequest->m_compilerArgs.IsEmpty() );

				ResourceCompilerArgument argument;

				argument.SetResourcePath(m_pRequest->m_compilerArgs);

				// Set force compilation flag
				argument.isForcedCompilation = false;
				if ( m_pRequest->RequiresForcedRecompiliation() )
				{
					argument.isForcedCompilation = true;
				}

				// Set package flag for packing request
				argument.isForPackagedBuild = false;
				if ( m_pRequest->m_origin == ResCompilationRequest::Origin::Package )
				{
					argument.isForPackagedBuild = true;
				}

				// Start compiler
				//-------------------------------------------------------------------------
				m_pRequest->m_status = ResCompilationRequest::Status::Compiling;
				m_pRequest->m_compilationTimeStarted = PlatformClock::GetTime();

				ResourceCompilerSystem* compiler = New<ResourceCompilerSystem>();

				if (!compiler->Initialize(m_context, argument))
				{
					m_pRequest->m_status = ResCompilationRequest::Status::Failed;
					m_pRequest->m_log = "Resource compiler failed to start!";
					m_pRequest->m_compilationTimeFinished = PlatformClock::GetTime();

					compiler->Shutdown();
					Delete(compiler);
				}

				Editor::CompilationResult compilationResult = compiler->Compile();
				if (compilationResult == Editor::CompilationResult::Failure)
				{
					m_pRequest->m_status = ResCompilationRequest::Status::Failed;
					m_pRequest->m_log = "Resource compiler failed to complete!";
					m_pRequest->m_compilationTimeFinished = PlatformClock::GetTime();

					compiler->Shutdown();
					Delete(compiler);
				}

				// Handle completed compilation
				//-------------------------------------------------------------------------
				m_pRequest->m_compilationTimeFinished = Platform::GetTimeSeconds();
				switch ( compilationResult )
				{
				case Editor::CompilationResult::SuccessUpToDate:
				{
					m_pRequest->m_status = ResCompilationRequest::Status::SucceededUpToDate;
				}
					break;

				case Editor::CompilationResult::Success:
				{
					m_pRequest->m_status = ResCompilationRequest::Status::Succeeded;
				}
					break;

				case Editor::CompilationResult::SuccessWithWarnings:
				{
					m_pRequest->m_status = ResCompilationRequest::Status::SucceededWithWarnings;
				}
					break;

				default:
				{
					m_pRequest->m_status = ResCompilationRequest::Status::Failed;
				}
					break;
				}

				// Read error
				//-------------------------------------------------------------------------
				// Strip the process preamble and delimiter
				uint64 const delimiterPos = m_pRequest->m_log.FindFirstOf(Editor::CompilationLog::s_delimiter);
				if ( delimiterPos != INVALID_INDEX)
				{
					uint64 const delimiterLength = StringUtils::Length(Editor::CompilationLog::s_delimiter);
					m_pRequest->m_log = m_pRequest->m_log.Substring( delimiterLength + 1, m_pRequest->m_log.Length() - delimiterLength - 1 );
				}

				compiler->Shutdown();
				Delete(compiler);
			}

			return true;
		}

	private:

		ResourceServerContext const&                        m_context;
		ResCompilationRequest*                                 m_pRequest = nullptr;
	};


	class PackagingTask final : public Threading::ThreadPoolTask
	{
	public:

		PackagingTask( ResourceServerContext const& context, List<ResID > const& mapsToBePackaged )
			: m_context( context )
			, m_mapsToBePackaged( mapsToBePackaged )
		{

		}

		inline List<ResID > const& GetRuntimeDependencies() const { return m_runtimeDependencies; }

	private:

		virtual bool Run() override
		{
			RuntimeModule::GetListOfAllRequiredModuleResources( m_runtimeDependencies );
			// EditorModule::GetListOfAllRequiredModuleResources( m_runtimeDependencies );

			//-------------------------------------------------------------------------

			for ( auto const& mapID : m_mapsToBePackaged )
			{
				EnqueueResourceForPackaging( mapID );
			}

			return true;
		}

		void EnqueueResourceForPackaging( ResID  const& resourceID )
		{
			if ( m_context.isExiting )
			{
				return;
			}

			//-------------------------------------------------------------------------

			auto pCompiler = m_context.pCompiler->GetCompilerForResourceType( resourceID.GetResourceTypeID() );
			if ( pCompiler != nullptr )
			{
				// Add resource for packaging
				m_runtimeDependencies.AddUnique(resourceID );

				// Get all runtime install dependencies
				List<ResID > referencedResources;
				pCompiler->GetInstallDependencies( resourceID, referencedResources );

				// Recursively enqueue all referenced resources
				for ( auto const& referenceResourceID : referencedResources )
				{
					EnqueueResourceForPackaging( referenceResourceID );
				}
			}
		}

	public:

		ResourceServerContext const&           m_context;
		List<ResID > const&              m_mapsToBePackaged;
		List<ResID>                     m_runtimeDependencies;
	};


	class ServerTask final : public Threading::ThreadPoolTask
	{
	public:

		void Sent(ResourceRequest* request)
		{
			m_PendingRequests.Add(request);
		}

		void Sent(ResID const& resourceID, ResCompilationRequest::Origin origin = ResCompilationRequest::Origin::External)
		{
			m_ManualPendingRequests.Add({resourceID, origin});
		}

		ConcurrentQueue<Pair<ResourceMessageID, ResourceResponse>> responseRequests;

		// Get the current list of maps queued to be packaged
		List<ResID> const& GetMapsQueuedForPackaging() const { return m_mapsToBePackaged; }

		// Are we currently packaging a map
		inline bool IsPackaging() const { return m_packagingStage != PackagingStage::None && m_packagingStage != PackagingStage::Complete; }

		// Get the current stage of packaging
		PackagingStage GetPackagingStage() const { return m_packagingStage; }

		// Packaging
		//-------------------------------------------------------------------------

		// Refresh the list of available maps to package
		void RefreshAvailableMapList()
		{
			m_allMaps.Clear();

			List<String> results;
			if (FileSystem::DirectoryGetFiles(results, m_Context.rawResourcePath, SE_TEXT("*.map"), DirectorySearchOption::All))
			{
				for ( auto const& foundMapPath : results )
				{
					m_allMaps.Add( ResID::FromFileSystemPath(m_Context.rawResourcePath, foundMapPath) );
				}
			}
		}

		// Get list of maps in the raw source folder
		List<ResID> const& GetAllFoundMaps() { return m_allMaps; }

		// Add map to the to-be-packaged list
		void AddMapToPackagingList( ResID mapResourceID )
		{
			//ENGINE_ASSERT(mapResourceID.GetResourceTypeID() == EntityModel::SerializedEntityMap::GetStaticResourceTypeID());
			m_mapsToBePackaged.AddUnique(mapResourceID);
		}

		// Remove map from the to-be-packaged list
		void RemoveMapFromPackagingList( ResID mapResourceID )
		{
			// ENGINE_ASSERT( mapResourceID.GetResourceTypeID() == EntityModel::SerializedEntityMap::GetStaticResourceTypeID() );
			m_mapsToBePackaged.Remove(mapResourceID);
		}

		// Do we have any maps on the to-be-packaged list
		bool CanStartPackaging() const
		{
			return (m_packagingStage == PackagingStage::None || m_packagingStage == PackagingStage::Complete ) && !m_mapsToBePackaged.IsEmpty();
		}

		// Start the packaging process
		void StartPackaging()
		{
			ENGINE_ASSERT( CanStartPackaging() );

			m_pPackagingTask = New<PackagingTask>( m_Context, m_mapsToBePackaged );
			m_pPackagingTask->Start();
			m_packagingStage = PackagingStage::Preparing;
		}

		// How far are we along with the packaging process
		float GetPackagingProgress() const
		{
			switch ( m_packagingStage )
			{
			case PackagingStage::None:
			{
				return 1.0f;
			}
				break;

			case PackagingStage::Preparing:
			{
				return 0.1f;
			}
				break;

			case PackagingStage::Packaging:
			{
				float numComplete = 0.0f;
				for ( auto pRequest : m_packagingRequests )
				{
					if ( pRequest->IsComplete() )
					{
						numComplete++;
					}
				}

				float const percentageComplete = numComplete / m_packagingRequests.Count();
				return 0.05f + ( 0.95f * percentageComplete );
			}
				break;

			case PackagingStage::Complete:
			{
				return 1.0f;
			}
				break;
			}

			return 0.0f;
		}

		ServerTask()
		{
			// File System
			//-------------------------------------------------------------------------
			m_fileSystemWatcher = New<FileSystemWatcher>(EngineContext::ProjectContentFolder, true);

			m_fileSystemWatcher->OnEvent.Bind<ServerTask, &ServerTask::OnFileChange>(this);

			RefreshAvailableMapList();
		}

	private:

		ConcurrentQueue<ResourceRequest*> m_PendingRequests;
		ConcurrentQueue<Pair<ResID, ResCompilationRequest::Origin>> m_ManualPendingRequests;

		List<ResourceRequest*>      m_WaitRequests;    // Request that were sent but we're still waiting for a response

		String                                              m_errorMessage;
		bool                                                m_cleanupRequested = false;

		// Compilation Requests
		List<ResCompilationRequest*>                        m_requests;
		List<CompilationTask*>                              m_activeTasks;
		std::atomic<int64>                                	m_numScheduledTasks = 0;

		List<FileWatcherEvent>								m_FileEvent;

		// Packaging
		List<ResID>                                         m_allMaps;
		List<ResID>                                         m_mapsToBePackaged;
		List<ResCompilationRequest const*>                  m_packagingRequests;
		PackagingTask*                                      m_pPackagingTask = nullptr;
		PackagingStage                                      m_packagingStage = PackagingStage::None;

		// File System Watcher
		FileSystemWatcher*                                  m_fileSystemWatcher;

		ResourceServerContext								m_Context;

	protected:
		bool Run() override
		{
			while (true)
			{
				// Update File System Watcher
				//-------------------------------------------------------------------------
				if (!m_FileEvent.IsEmpty())
				{
					for (FileWatcherEvent const& fsEvent : m_FileEvent )
					{
						if (fsEvent.action != FileSystemAction::Modify)
						{
							continue;
						}

						//-------------------------------------------------------------------------

						ENGINE_ASSERT(!fsEvent.path.IsEmpty());

						ResPath resourcePath = ResPath::FromFileSystemPath(m_Context.rawResourcePath, fsEvent.path);
						if (!resourcePath.IsValid())
						{
							continue;
						}

						ResID resourceID( resourcePath );
						if (!resourceID.IsValid())
						{
							continue;
						}

						// 如果有记录，重新编译
						CreateResourceRequest(resourceID, ResCompilationRequest::Origin::FileWatcher);
					}
				}

				// Send all requests and keep-alive messages
				//-------------------------------------------------------------------------
				ResourceRequest* request;
				while (m_PendingRequests.try_dequeue(request))
				{
					m_WaitRequests.Add(request);

					CreateResourceRequest(request->GetResourceID());
				}


				Pair<ResID, ResCompilationRequest::Origin> manualRequest;
				while (m_ManualPendingRequests.try_dequeue(manualRequest))
				{
					CreateResourceRequest(manualRequest.First, manualRequest.Second);
				}

				// Update Packaging
				//-------------------------------------------------------------------------

				if (m_packagingStage == PackagingStage::Preparing)
				{
					ENGINE_ASSERT(m_pPackagingTask != nullptr);

					if (m_pPackagingTask->IsFinished())
					{
						for (auto const &resourceID : m_pPackagingTask->GetRuntimeDependencies())
						{
							m_packagingRequests.Add(CreateResourceRequest(resourceID, ResCompilationRequest::Origin::Package));
						}

						Delete(m_pPackagingTask);
						m_packagingStage = PackagingStage::Packaging;
					}
				}
				else if (m_packagingStage == PackagingStage::Packaging)
				{
					bool isComplete = true;

					for (auto pRequest : m_packagingRequests)
					{
						if (!pRequest->IsComplete())
						{
							isComplete = false;
							break;
						}
					}

					if (isComplete)
					{
						m_packagingRequests.Clear();
						m_packagingStage = PackagingStage::Complete;
					}
				}

				ProcessCompletedRequests();
			}

			return true;
		}

		ResCompilationRequest* CreateResourceRequest( ResID const& resourceID, ResCompilationRequest::Origin origin = ResCompilationRequest::Origin::External)
		{
			ResCompilationRequest* pRequest = New<ResCompilationRequest>();

			if ( resourceID.IsValid() )
			{
				//-------------------------------------------------------------------------
				pRequest->m_origin = origin;
				pRequest->m_resourceID = resourceID;
				pRequest->m_sourceFile = ResPath::ToFileSystemPath(m_Context.rawResourcePath, pRequest->m_resourceID.GetResourcePath() );
				pRequest->m_compilerArgs = pRequest->m_resourceID.GetResourcePath().c_str();
				pRequest->m_status = ResCompilationRequest::Status::Pending;

				// Set the destination path based on request type
				if ( origin == ResCompilationRequest::Origin::Package )
				{
					pRequest->m_destinationFile = ResPath::ToFileSystemPath(m_Context.packagedBuildCompiledResourcePath, pRequest->m_resourceID.GetResourcePath() );
				}
				else
				{
					pRequest->m_destinationFile = ResPath::ToFileSystemPath(m_Context.compiledResourcePath, pRequest->m_resourceID.GetResourcePath() );
				}
			}
			else // Invalid resource ID
			{
				pRequest->m_log = String::Format(SE_TEXT("Invalid resource ID ( {} )"), resourceID.c_str() );
				pRequest->m_status = ResCompilationRequest::Status::Failed;
			}

			// Enqueue new request
			//-------------------------------------------------------------------------
			auto pTask = New<CompilationTask>(m_Context, pRequest);
			pTask->Start();

			m_requests.Add(pRequest);
			m_activeTasks.Add(pTask);
			m_numScheduledTasks++;
			return pRequest;
		}

		void ProcessCompletedRequests()
		{
			List<Pair<ResourceMessageID, ResourceResponse>> response;

			// Fill buckets
			//-------------------------------------------------------------------------
			for (int32 i = m_activeTasks.Count() - 1; i >= 0; i--)
			{
				CompilationTask* pActiveTask = m_activeTasks[i];

				if (pActiveTask->IsEnded())
				{
					auto pRequest = pActiveTask->GetRequest();
					ENGINE_ASSERT(pRequest->IsComplete());

					// No notifications if exiting
					if ( !m_Context.isExiting )
					{
						// Notify all clients
						if (pRequest->IsInternalRequest())
						{
							// No need to notify the client for internal requests resources that are up to date
							if ( pRequest->m_status != ResCompilationRequest::Status::SucceededUpToDate )
							{
								ResourceResponse resourceResponse;
								if (pRequest->HasSucceeded() )
								{
									resourceResponse = {pRequest->GetResourceID(), pRequest->GetDestinationFilePath().ToString()};
								}
								else
								{
									resourceResponse = { pRequest->GetResourceID(), SE_TEXT(""), pRequest->GetLog() };
								}

								response.Add({ ResourceMessageID::ResourceUpdated, resourceResponse});
							}
						}
						else
						{
							ResourceResponse resourceResponse;
							if (pRequest->HasSucceeded() )
							{
								resourceResponse = {pRequest->GetResourceID(), pRequest->GetDestinationFilePath().ToString()};
							}
							else
							{
								resourceResponse = { pRequest->GetResourceID(), SE_TEXT(""), pRequest->GetLog() };
							}

							response.Add({ ResourceMessageID::ResourceRequestComplete, resourceResponse});
						}
					}

					// Delete task
					Delete( pActiveTask );
					m_activeTasks.RemoveAt(i);

					// Decrement task counter
					m_numScheduledTasks--;
				}
			}

			for (auto t : response)
			{
				responseRequests.Add(t);
			}
		}

		void OnFileChange(FileWatcherEvent &event)
		{
			m_FileEvent.Add(event);
		}
	};


    bool ResourceServerProvider::IsReady() const
    {
        return m_ServerTask != nullptr && m_ServerTask->IsRunning();
    }

    bool ResourceServerProvider::Initialize()
    {
		String compiledResourceDatabasePath = EngineContext::ProjectFolder + SE_TEXT("/") + m_settings.m_compiledResourceDatabasePath;

		m_CompiledResourceDatabase = New<CompiledResourceDatabase>();

		if (!m_CompiledResourceDatabase->Connect(compiledResourceDatabasePath))
		{
			LOG_ERROR("Resource", "CompiledResourceDatabase connect failed {0}", m_CompiledResourceDatabase->GetError());
			Delete(m_CompiledResourceDatabase);
			return false;
		}

		m_Context.rawResourcePath = EngineContext::ProjectContentFolder;
		m_Context.compiledResourcePath = EngineContext::ProjectCacheFolder;
		m_Context.compiledResourceDatabasePath = compiledResourceDatabasePath;
		m_Context.pCompiler = New<CompilerRegistry>(EngineContext::ProjectContentFolder);;

		m_ServerTask = New<ServerTask>();
		m_ServerTask->Start();
        return true;
    }

    void ResourceServerProvider::Shutdown()
    {
		m_ServerTask->Wait(10000);

		Delete(m_ServerTask);
    }

    void ResourceServerProvider::RequestRawResource(ResourceRequest *pRequest)
    {
        ENGINE_ASSERT(pRequest != nullptr && pRequest->IsValid() && pRequest->GetLoadingStatus() == LoadingStatus::Loading);

		ResID id = pRequest->GetResourceID();
		int32 foundIndex = ListExtensions::IndexOf(m_sentRequests,
			{[id](ResourceRequest * const& pRequest){ return pRequest->GetResourceID() == id; }});

		ENGINE_ASSERT(foundIndex == INVALID_INDEX);
        //-------------------------------------------------------------------------
		m_ServerTask->Sent(pRequest);
		m_sentRequests.Add(pRequest);
    }

    void ResourceServerProvider::CancelRequest(ResourceRequest *pRequest)
    {
        ENGINE_ASSERT(pRequest != nullptr && pRequest->IsValid());

        auto foundIter = m_sentRequests.Find(pRequest);
        ENGINE_ASSERT(foundIter != INVALID_INDEX);

        m_sentRequests.RemoveAt(foundIter);
    }

	void ResourceServerProvider::CompileResource(const ResID& resourceID, bool forceRecompile)
	{
		m_ServerTask->Sent(resourceID, forceRecompile ? ResCompilationRequest::Origin::ManualCompileForced : ResCompilationRequest::Origin::ManualCompile );
	}

	void ResourceServerProvider::PackageResource(const ResID& resourceID)
	{
		m_ServerTask->Sent( resourceID, ResCompilationRequest::Origin::Package );
	}

    void ResourceServerProvider::Update()
    {
		PROFILE_CPU()

        //-------------------------------------------------------------------------

#ifdef SE_DEVELOPMENT
        m_externallyUpdatedResources.Clear();
#endif

        // Process all server results
        //-------------------------------------------------------------------------
		Pair<ResourceMessageID, ResourceResponse> result;
		while (m_ServerTask->responseRequests.try_dequeue(result))
		{
			ResourceResponse &response = result.Second;
			ResID id = response.resourceID;

			auto foundIter = ListExtensions::IndexOf(m_sentRequests, {[id](ResourceRequest * const& pRequest)
			{
			  	return pRequest->GetResourceID() == id;
			}});

			// This might have been a canceled request
			if (foundIter == INVALID_INDEX)
			{
				continue;
			}

			ResourceRequest *pFoundRequest = m_sentRequests[foundIter];
			ENGINE_ASSERT(pFoundRequest->IsValid());

			// 忽略对可能已取消或正在卸载的请求的任何响应
			if (pFoundRequest->GetLoadingStatus() != LoadingStatus::Loading)
			{
				continue;
			}

			// 失败时触发通知
			if (response.filePath.IsEmpty())
			{
				NOTIFY_ERROR_FORMAT(SE_TEXT("{0} failed to Compile!\n\nLog: {1}"), response.resourceID.c_str(), response.log);
			}

			// If the request has a filepath set, the compilation was a success
			pFoundRequest->OnRawResourceRequestComplete(response.filePath, response.log);

			// Remove from request list
			m_sentRequests.RemoveAt(foundIter);
		}
    }
}