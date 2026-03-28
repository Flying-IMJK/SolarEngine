
#include "RenderOutputViewport.h"

#include "Runtime/Engine.h"
#include "Runtime/Graphics/GPUDevice.h"
#include "Runtime/Graphics/Textures/GPUTexture.h"
#include "Runtime/Render/RenderTask.h"
#include "Runtime/Render/2D/Render2D.h"
#include "Runtime/Utilities/Time.h"
#include "Runtime/UI/GUI/RootControl.h"

namespace SE::Editor
{
	SceneRenderTask* RenderOutputViewport::GetTask()
	{
		return m_Task;
	}

	RenderOutputViewport::RenderOutputViewport(SceneRenderTask* task)
	{
		ENGINE_ASSERT(task != nullptr);
		m_Task = task;

		m_BackBuffer = GPUDevice::instance->CreateTexture(SE_TEXT("RenderOutputControl.BackBuffer"));

		m_ResizeTime = ResizeCheckTime;

		m_Task->Output = m_BackBuffer;
		m_Task->End.BindUnique<RenderOutputViewport, &RenderOutputViewport::OnEnd>(this);

		Engine::UpdateCall.BindUnique<RenderOutputViewport, &RenderOutputViewport::OnUpdate>(this);
	}

	void RenderOutputViewport::Draw()
	{
		Rectangle bounds = Rectangle(Float2::Zero, Size);

		// Draw background
		Color backgroundColor = BackgroundColor;
		if (backgroundColor.a > 0.0f)
		{
			Render2D::FillRectangle(bounds, backgroundColor);
		}

		// Draw backbuffer texture
		GPUTexture* buffer = m_BackBufferOld ? m_BackBufferOld : m_BackBuffer;
		Color color = TintColor.RGBMultiplied(Brightness);
		if (KeepAspectRatio)
		{
			float ratioX = bounds.GetWidth() / buffer->Width();
			float ratioY = bounds.GetHeight() / buffer->Height();
			float ratio = ratioX < ratioY ? ratioX : ratioY;
			bounds = Rectangle(
				(bounds.GetWidth() - buffer->Width() * ratio) / 2,
				(bounds.GetHeight() - buffer->Height() * ratio) / 2,
				buffer->Width() * ratio,
				buffer->Height() * ratio);
		}
		Render2D::DrawTexture(buffer, bounds, color);

		// Push clipping mask
		if (ClipChildren)
		{
			Rectangle clientArea = GetDesireClientArea();
			Render2D::PushClip(clientArea);
		}

		DrawChildren();

		// Pop clipping mask
		if (ClipChildren)
		{
			Render2D::PopClip();
		}
	}

	void RenderOutputViewport::SyncBackbufferSize()
	{
		float scale = ResolutionScale * GetDpiScale();
		int width = Math::CeilToInt(Width * scale);
		int height = Math::CeilToInt(Height * scale);
		if (_customResolution != Float2::Zero)
		{
			width = _customResolution.x;
			height = _customResolution.y;
		}
		if (m_BackBuffer == nullptr || m_BackBuffer->Width() == width && m_BackBuffer->Height() == height)
		{
			return;
		}
		if (width < 1 || height < 1)
		{
			m_BackBuffer->ReleaseGPU();

			SAFE_DELETE(m_BackBufferOld);
			return;
		}

		// Cache old backbuffer to remove flickering effect
		if (m_BackBufferOld == nullptr && m_BackBuffer->IsAllocated())
		{
			m_BackBufferOld = m_BackBuffer;
			m_BackBuffer = GPUDevice::instance->CreateTexture(SE_TEXT("RenderOutputControl.BackBuffer"));
		}

		// Set timeout to remove old buffer
		_oldBackbufferLiveTimeLeft = 3;

		// Resize backbuffer
		GPUTextureDescription desc = GPUTextureDescription::New2D(width, height, BackBufferFormat);
		m_BackBuffer->Init(desc);
		m_Task->Output = m_BackBuffer;
	}

	void RenderOutputViewport::OnDestroy()
	{
		if (IsDisposing)
			return;

		Engine::UpdateCall.Unbind<RenderOutputViewport, &RenderOutputViewport::OnUpdate>(this);

		// Cleanup
		// Scripting.Update -= OnUpdate;
		if (m_Task != nullptr)
		{
			m_Task->Enabled = false;
			// _task->ClearCustomActors();
			//_task.CustomPostFx.Clear();
		}
		m_BackBuffer->ReleaseGPU();
		m_BackBufferOld->ReleaseGPU();
		SAFE_DELETE(m_BackBuffer);
		SAFE_DELETE(m_BackBufferOld);
		SAFE_DELETE(m_Task);

		ContainerControl::OnDestroy();
	}

	bool RenderOutputViewport::WalkTree(Control* c)
	{
		while (c != nullptr)
		{
			if (TypeAs<RootControl>(c))
			{
				return false;
			}
			if (c->Visible == false)
			{
				break;
			}
			c = c->Parent;
		}
		return true;
	}

	void RenderOutputViewport::OnUpdate()
	{
		if (m_Task == nullptr || !UseAutomaticTaskManagement)
		{
			return;
		}
		float deltaTime = Time::GetUnscaledDeltaTime();

		// Check if need to resize the output
		m_ResizeTime += deltaTime;
		if (m_ResizeTime >= ResizeCheckTime && Visible && Enabled)
		{
			m_ResizeTime = 0;
			SyncBackbufferSize();
		}

		// Check if skip rendering
		bool wasEnabled = m_Task->Enabled;
		m_Task->Enabled = !CanSkipRendering();
		if (!wasEnabled && m_Task->Enabled)
		{
			SyncBackbufferSize();
		}
	}

	bool RenderOutputViewport::CanSkipRendering()
	{
		// Disable task rendering if control is very small
		const float MinRenderSize = 4;
		if (Width < MinRenderSize || Height < MinRenderSize)
			return true;

		// Disable task rendering if control is not used in a window (has using ParentWindow)
		if (RenderOnlyWithWindow)
		{
			return WalkTree(Parent);
		}

		return false;
	}

	void RenderOutputViewport::OnEnd(RenderTask* task, GPUContext* context)
	{
		// Check if was using old backbuffer
		if (m_BackBufferOld)
		{
			_oldBackbufferLiveTimeLeft--;
			if (_oldBackbufferLiveTimeLeft < 0)
			{
				m_BackBufferOld->ReleaseGPU();
				SAFE_DELETE(m_BackBufferOld);
			}
		}
	}

	Int2 RenderOutputViewport::__GetCustomResolution()
	{
		return _customResolution;
	}

	void RenderOutputViewport::__SetCustomResolution(Int2 value)
	{
		if (_customResolution != value)
		{
			_customResolution = value;
			SyncBackbufferSize();
		}
	}
} // SE