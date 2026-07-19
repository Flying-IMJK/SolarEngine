
#include "GPUSwapChain.h"
#include "Runtime/Core/Thread/Task.h"
#include "Textures/GPUTexture.h"
#include "GPUDevice.h"

namespace SE
{
	class GPUSwapChainDownloadTask : public Threading::Task
	{
		friend GPUSwapChain;

	public:
		GPUSwapChain* SwapChain;
		GPUTexture* Texture;

		~GPUSwapChainDownloadTask()
		{
			DeleteObjectSafe(Texture);
		}

		String ToString() const override
		{
			return SE_TEXT("GPUSwapChainDownloadTask");
		}

		bool Run() override
		{
			return false;
		}

		void Enqueue() override
		{
		}
	};

	GPUSwapChain::GPUSwapChain()
		: GPUResource()
	{
#if GPU_ENABLE_RESOURCE_NAMING
//		SetName(SE_TEXT("Swap Chain (backbuffers)"));
#endif
	}

	Task* GPUSwapChain::DownloadDataAsync(TextureData& result)
	{
		if (_downloadTask)
		{
			LOG_WARNING("Graphic", "Can download window backuffer data only once at the time.");
			return nullptr;
		}

		auto texture = GPUDevice::instance->CreateTexture();
		if (texture->Init(GPUTextureDescription::New2D(GetWidth(), GetHeight(), GetFormat(), GPUTextureFlags::None, 1).ToStagingReadback()))
		{
			LOG_WARNING("Graphic", "Failed to create staging texture for the window swapchain backuffer download.");
			return nullptr;
		}

		auto task = New<GPUSwapChainDownloadTask>();
		task->SwapChain = this;
		task->Texture = texture;
/*
		const auto downloadTask = texture->DownloadDataAsync(result);
		task->ContinueWith(downloadTask);*/

		_downloadTask = reinterpret_cast<Task*>(task);
		return _downloadTask;
	}

	void GPUSwapChain::End()
	{
		if (_downloadTask)
		{
			auto downloadTask = (GPUSwapChainDownloadTask*)_downloadTask;
			auto context = GPUDevice::instance->GetMainContext();
//			CopyBackbuffer(context, downloadTask->Texture);
			downloadTask->Execute();
			_downloadTask = nullptr;
		}
	}

	void GPUSwapChain::Present(bool vsync)
	{
		// Check if need to show the window after the 1st paint
		if (_window->m_ShowAfterFirstPaint)
		{
			_window->m_ShowAfterFirstPaint = false;
			_window->Show();
		}

		// Count amount of present calls
		_presentCount++;
	}

	String GPUSwapChain::ToString() const
	{
#if GPU_ENABLE_RESOURCE_NAMING
		return String::Format(SE_TEXT("SwapChain {0}x{1}, {2}"), GetWidth(), GetHeight(), String("GetName()"));
#else
		return SE_TEXT("SwapChain");
#endif
	}

	GPUResourceType GPUSwapChain::GetResType() const
	{
		return GPUResourceType::Texture;
	}

	void GPUSwapChain::OnDeleteObject()
	{
		_window = nullptr;
		GPUResource::OnDeleteObject();
	}

	void GPUSwapChain::OnReleaseGPU()
	{
		_downloadTask = nullptr;
		GPUResource::OnReleaseGPU();
	}
}
