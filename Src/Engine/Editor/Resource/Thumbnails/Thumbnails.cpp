

#include "Thumbnails.h"

#include "PreviewsCache.h"
#include "ThumbnailRequest.h"
#include "Runtime/Core/Logging/Exception.h"
#include "Runtime/Core/Platform/Windows/WindowsFileSystem.h"
#include "Runtime/Core/Thread/Threading.h"
#include "Editor/EditorApp.h"
#include "Editor/Modules/AssetDatabaseModule.h"
#include "Editor/Resource/Items/AssetItem.h"
#include "Editor/Resource/Opreate/AssetOperate.h"
#include "Runtime/EngineContext.h"
#include "Runtime/Graphics/GPUContext.h"
#include "Runtime/Graphics/GPUDevice.h"
#include "Runtime/Graphics/Textures/GPUTexture.h"
#include "Runtime/Render/RenderTask.h"
#include "Runtime/Render/2D/Render2D.h"
#include "Runtime/Utilities/Time.h"

namespace SE::Editor
{
	Thumbnails::Thumbnails(EditorApp* editor) : editor(editor)
	{
		m_GuiRoot = New<PreviewRoot>();
		_cacheFolder = FileSystem::CombinePaths(EngineContext::ProjectCacheFolder, SE_TEXT("Thumbnails"));
		_lastFlushTime = DateTime::Now();
	}

	Thumbnails::~Thumbnails()
	{
		Delete(m_GuiRoot);
	}

	void Thumbnails::RequestPreview(ContentItem* item)
	{
		ENGINE_ASSERT(item != nullptr);

		// Check if use default icon
		if (item->HasDefaultThumbnail)
		{
			item->Thumbnail = item->DefaultThumbnail;
			return;
		}

		// We cache previews only for items with 'ID', for now we support only AssetItems
		AssetItem* assetItem = TypeTryCast<AssetItem>(item);
		if (assetItem == nullptr)
			return;

		// Ensure that there is valid proxy for that item
		AssetOperate* proxy = TypeTryCast<AssetOperate>(editor->databaseModule->GetProxy(item));
		if (proxy == nullptr)
		{
			LOG_WARNING("Asset", "Cannot generate preview for item {0}. Cannot find proxy for it.", item->Path);
			return;
		}

		{
			Threading::ScopeLock lock (_CriticalSection);
			// Check if element hasn't been already processed for generating preview
			if (FindRequest(assetItem) == nullptr)
			{
				// Check each cache atlas
				for (int i = 0; i < m_Cache.Count(); i++)
				{
					SpriteHandle sprite = m_Cache[i]->FindSlot(assetItem->id);
					if (sprite.IsValid())
					{
						// Found!
						item->Thumbnail = sprite;
						return;
					}
				}

				AddRequest(assetItem, proxy);
			}
		}
	}

	void Thumbnails::DeletePreview(ContentItem* item)
	{
		ENGINE_ASSERT(item != nullptr);

		// We cache previews only for items with 'ID', for now we support only AssetItems
		AssetItem* assetItem = TypeTryCast<AssetItem>(item);
		if (assetItem == nullptr)
			return;

		Threading::ScopeLock lock (_CriticalSection);
		// Cancel loading
		RemoveRequest(assetItem);

		// Find atlas with preview and remove it
		for (int i = 0; i < m_Cache.Count(); i++)
		{
			if (m_Cache[i]->ReleaseSlot(assetItem->id))
				break;
		}
	}

	void Thumbnails::OnItemDispose(ContentItem* item)
	{
		AssetItem* assetItem = TypeTryCast<AssetItem>(item);
		if (assetItem == nullptr)
		{
			Threading::ScopeLock lock (_CriticalSection);
			RemoveRequest(assetItem);
		}
	}

	void Thumbnails::OnInit()
	{
		// Create cache folder
		if (!FileSystem::DirectoryExists(_cacheFolder))
		{
			FileSystem::CreateDirectory(_cacheFolder);
		}

		// Find atlases in a Editor cache directory
		List<String> files;
		FileSystem::DirectoryGetFiles(files, _cacheFolder, SE_TEXT("cache_*.sge"), DirectorySearchOption::TopOnly);
		int atlases = 0;
		for (int i = 0; i < files.Count(); i++)
		{
			// Load asset
			Asset* asset = AssetContent::LoadAsync<Asset>(files[i]);
			if (asset == nullptr)
				continue;

			PreviewsCache* atlas = TypeTryCast<PreviewsCache>(asset);
			// Validate type
			if (atlas != nullptr)
			{
				// Cache atlas
				atlases++;
				m_Cache.Add(atlas);
			}
			else
			{
				// Skip asset
				LOG_WARNING("Asset", "Asset \'{0}\' is inside Editor\'s private directory for Assets Thumbnails Cache. Please move it.", asset->GetPath());
			}
		}
		LOG_INFO("Asset", "Previews cache count: {0} (capacity for {1} icons)", atlases, atlases * PreviewsCache::AssetIconsPerAtlas);

		// Prepare at least one atlas
		if (m_Cache.Count() == 0)
		{
			GetValidAtlas();
		}

		// Create render task but disabled for now
		_output = GPUDevice::instance->CreateTexture(SE_TEXT("ThumbnailsOutput"));
		GPUTextureDescription desc = GPUTextureDescription::New2D(PreviewsCache::AssetIconSize, PreviewsCache::AssetIconSize, PreviewsCache::AssetIconsAtlasFormat);
		_output->Init(desc);
		_task = New<RenderTask>();
		_task->Order = 50; // Render this task later
		_task->Enabled = false;
		_task->Render.Bind<Thumbnails, &Thumbnails::OnRender>(this);
	}

	void Thumbnails::OnUpdate()
	{
		// Wait some frames before start generating previews (late init feature)
		if (Time::GetTimeSinceStartup() < 1.0f || HasAllAtlasesLoaded() == false)
		{
			return;
		}

		{
			Threading::ScopeLock lock (_CriticalSection);
			DateTime now = DateTime::Now();

			// Check if has any request pending
			int count = _requests.Count();
			if (count > 0)
			{
				// Prepare requests
				bool isAnyReady = false;
				int checks = Math::Min(10, count);
				for (int i = 0; i < checks; i++)
				{
					ThumbnailRequest* request = _requests[i];

					request->Update();
					if (request->IsReady)
					{
						isAnyReady = true;
					}
					else if (request->state == ThumbnailRequest::States::Created)
					{
						request->Prepare();
					}
					else if (request->state == ThumbnailRequest::States::Failed)
					{
						_requests.RemoveAt(i--);
						checks--;
					}
				}

				// Check if has no rendering task enabled but should be
				if (isAnyReady && _task->Enabled == false)
				{
					// Start generating preview
					StartPreviewsQueue();
				}
			}
			// Don't flush every frame
			else if (now - _lastFlushTime >= TimeSpan::FromSeconds(1))
			{
				// Flush data
				_lastFlushTime = now;
				Flush();
			}
		}
	}

	void Thumbnails::OnExit()
	{
		if (_task)
			_task->Enabled = false;

		{
			Threading::ScopeLock lock (_CriticalSection);
			// Clear data
			while (_requests.Count() > 0)
			{
				RemoveRequest(_requests[0]);
			}
			m_Cache.Clear();
		}

		m_GuiRoot->Dispose();
		Delete(_task);
		Delete(_output);
	}

	void Thumbnails::OnRender(RenderTask* task, GPUContext* context)
	{
		Threading::ScopeLock lock (_CriticalSection);

		// Check if there is ready next asset to render thumbnail for it
		// But don't check whole queue, only a few items
		ThumbnailRequest* request = GetReadyRequest(10);
		if (request == nullptr)
		{
			// Disable task
			_task->Enabled = false;
			return;
		}

		// Find atlas with an free slot
		PreviewsCache* atlas = GetValidAtlas();
		if (atlas == nullptr)
		{
			// Error
			_task->Enabled = false;
			_requests.Clear();
			LOG_ERROR("Asset", "Failed to get atlas.");
			return;
		}

		// Wait for atlas being loaded
		if (!atlas->IsReady())
			return;


		// Setup
		m_GuiRoot->RemoveChildren();
		m_GuiRoot->AccentColor = request->operate->AccentColor;

		// Call proxy to prepare for thumbnail rendering
		request->operate->OnThumbnailDrawBegin(request, m_GuiRoot, context);
		m_GuiRoot->UnlockChildrenRecursive();

		// Draw preview
		context->Clear(_output->View(), Colors::Black);
		Render2D::CallDrawing([this]()
		{
			m_GuiRoot->Draw();
		}, context, _output);

		// Call proxy and cleanup UI (delete create controls, shared controls should be unlinked during OnThumbnailDrawEnd event)
		request->operate->OnThumbnailDrawEnd(request, m_GuiRoot);

		m_GuiRoot->DisposeChildren();

		// Copy backbuffer with rendered preview into atlas
		SpriteHandle icon = atlas->OccupySlot(_output, request->Item->id);
		if (!icon.IsValid())
		{
			// Error
			_task->Enabled = false;
			_requests.Clear();
			LOG_ERROR("Asset", "Failed to occupy previews cache atlas slot.");
			return;
		}

		// End
		request->FinishRender(icon);
		RemoveRequest(request);
	}

	void Thumbnails::StartPreviewsQueue()
	{
		// Ensure to have valid atlas
		GetValidAtlas();

		// Enable task
		_task->Enabled = true;
	}


	ThumbnailRequest* Thumbnails::FindRequest(AssetItem* item)
	{
		for (int i = 0; i < _requests.Count(); i++)
		{
			if (_requests[i]->Item == item)
			{
				return _requests[i];
			}
		}
		return nullptr;
	}

	void Thumbnails::AddRequest(AssetItem* item, AssetOperate* proxy)
	{
		ThumbnailRequest* request = New<ThumbnailRequest>(item, proxy);
		_requests.Add(request);
		item->AddReference(this);
	}

	void Thumbnails::RemoveRequest(ThumbnailRequest* request)
	{
		request->Dispose();
		_requests.Remove(request);
		request->Item->RemoveReference(this);
		Delete(request);
	}

	void Thumbnails::RemoveRequest(AssetItem* item)
	{
		ThumbnailRequest* request = FindRequest(item);
		if (request != nullptr)
			RemoveRequest(request);
	}

	ThumbnailRequest* Thumbnails::GetReadyRequest(int maxChecks)
	{
		maxChecks = Math::Min(maxChecks, _requests.Count());
		for (int i = 0; i < maxChecks; i++)
		{
			ThumbnailRequest* request = _requests[i];
			if (request->IsReady)
				return request;
		}

		return nullptr;
	}

	PreviewsCache* Thumbnails::CreateAtlas()
	{
		// Create atlas path
		String path = FileSystem::CombinePaths(_cacheFolder, String::Format(SE_TEXT("cache_{0}.{1}"), UID::New(), ASSET_FILES_EXTENSION));

		// Create atlas
		if (!PreviewsCache::Create(path))
		{
			LOG_ERROR("Asset", "Failed to create thumbnails atlas.");
			return nullptr;
		}

		// Load atlas
		PreviewsCache* atlas = AssetContent::LoadAsync<PreviewsCache>(path);
		if (atlas == nullptr)
		{
			LOG_ERROR("Asset", "Failed to load thumbnails atlas.");
			return nullptr;
		}

		// Register new atlas
		m_Cache.Add(atlas);

		return atlas;
	}

	void Thumbnails::Flush()
	{
		for (int i = 0; i < m_Cache.Count(); i++)
		{
			m_Cache[i]->Flush();
		}
	}

	bool Thumbnails::HasAllAtlasesLoaded()
	{
		for (int i = 0; i < m_Cache.Count(); i++)
		{
			if (!m_Cache[i]->IsReady())
			{
				return false;
			}
		}
		return true;
	}

	PreviewsCache* Thumbnails::GetValidAtlas()
	{
		// Check if has no free slots
		for (int i = 0; i < m_Cache.Count(); i++)
		{
			if (m_Cache[i]->HasFreeSlot())
			{
				return m_Cache[i];
			}
		}

		// Create new atlas
		return CreateAtlas();
	}

	PreviewRoot::PreviewRoot() : ContainerControl(0, 0, PreviewsCache::AssetIconSize, PreviewsCache::AssetIconSize)
	{
		AutoFocus = false;
		AccentColor = Colors::Pink;
		SetIsLayoutLocked(false);
	}

	void PreviewRoot::Draw()
	{
		ContainerControl::Draw();

		// Draw accent
		const float accentHeight = 2;
		Render2D::FillRectangle(Rectangle(0, Height - accentHeight, Width, accentHeight), AccentColor);
	}
} // SE