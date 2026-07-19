#include "GUIRenderer.h"

#include "Runtime/Core/Systems.h"
#include "Runtime/Core/Memory/Memory.h"
#include "Runtime/Core/Types/Strings/String.h"
#include "Runtime/Core/Thread/Threading.h"

#include "Runtime/Core/Platform/FileSystem.h"
#include "Runtime/Core/Serialization/MemoryWriteStream.h"
#include "Runtime/Core/Platform/Window.h"
#include "Runtime/Core/Profiler/Profiler.h"

#include "Runtime/Graphics/GPUContext.h"
#include "Runtime/Graphics/DynamicBuffer.h"
#include "Runtime/Graphics/GPUDevice.h"
#include "Runtime/Graphics/Base/GPUPipelineState.h"
#include "Runtime/Graphics/Textures/GPUSampler.h"
#include "Runtime/Graphics/Base/GPUEnums.h"
#include "Runtime/Graphics/Shaders/GPUShader.h"
#include "Runtime/Graphics/GPUSwapChain.h"
#include "Runtime/Graphics/GraphicWindow.h"
#include "Runtime/Graphics/Textures/GPUTexture.h"
#include "Runtime/Graphics/Textures/GPUTextureDescription.h"
#include "Runtime/EngineContext.h"
#include "Runtime/ShaderCompilation/ShadersCompilation.h"


// #include "Imgui/misc/freetype/imgui_freetype.h"
// #include "Imgui/imgui_impl_win32.h"
// #include "Imgui/imgui.h"
// #include "Imgui/imgui_internal.h"
#include "Runtime/Resource/AssetContent.h"
#include "Runtime/Resource/Assets/Materials/Shader.h"

namespace SE
{
	class RenderWindow
	{
		Window* window;
	};

    //-------------------------------------------------------------------------

/*    static void ImGui_CreateWindowContext(ImGuiViewport *pViewport)
    {
        ImGuiIO &io = ImGui::GetIO();
        
        //-------------------------------------------------------------------------

        void* hwnd = pViewport->PlatformHandleRaw ? pViewport->PlatformHandleRaw : pViewport->PlatformHandle;
        ENGINE_ASSERT(hwnd != 0);


*//*        RHISwapChain::Desc desc;
        desc.width = (uint32)pViewport->Size.x;
        desc.height = (uint32)pViewport->Size.y;

        RHISwapChain* swapChain = New<RHISwapChain>();

        ENGINE_ASSERT(GetRHI()->CreateSwapchain(&desc, hwnd, swapChain));

        RHIRenderWindow* remderWindow = New<RHIRenderWindow>(swapChain);

        pViewport->RendererUserData = remderWindow;*//*
    }

    static void ImGui_DestroyWindowContext(ImGuiViewport *pViewport)
    {
        ImGuiIO &io = ImGui::GetIO();
        //-------------------------------------------------------------------------

        // The main viewport (owned by the application) will always have RendererUserData == NULL since we didn't create the data for it.
*//*        if (auto pSecondaryWindow = (RHIRenderWindow *)pViewport->RendererUserData)
        {
            Delete(pSecondaryWindow);
        }

        pViewport->RendererUserData = nullptr;*//*
    }

    static void ImGui_SetWindowSize(ImGuiViewport *pViewport, ImVec2 size)
    {
        ImGuiIO &io = ImGui::GetIO();
        //-------------------------------------------------------------------------
        void* hwnd = pViewport->PlatformHandleRaw ? pViewport->PlatformHandleRaw : pViewport->PlatformHandle;
        ENGINE_ASSERT(hwnd != 0);

        *//*auto pSecondaryWindow = (RHIRenderWindow *)pViewport->RendererUserData;

        RHISwapChain * swapChain = pSecondaryWindow->GetSwapChain();
        
        RHISwapChain::Desc desc = swapChain->desc;
        desc.width = (uint32)size.x;
        desc.height = (uint32)size.y;

        GetRHI()->CreateSwapchain(&desc, hwnd, swapChain);*//*
    }*/

    //-------------------------------------------------------------------------
	/*struct GUIRendererData
	{
		DynamicIndexBuffer* m_IndexBuffer;
		DynamicVertexBuffer* m_VertexBuffer;
		GPUTexture* m_FontTexture;
		GPUSampler* m_Sampler;
		GPUConstantBuffer* m_PreDraw;
		GPUPipelineState* m_PipelineState;

		Shader* m_Shader;
		StringAnsi m_iniFilename;
		StringAnsi m_logFilename;
	} *data;


	class GUIRendererSystem final : public ISystem
	{
	ENGINE_SYSTEM(GUIRendererSystem)

	public:
		GUIRendererSystem() : ISystem(SE_TEXT("GUI-Render"))
		{

		}

	protected:
		bool OnInit() override
		{
			data = New<GUIRendererData>();

			ImGui::CreateContext();

			ImGuiIO &io = ImGui::GetIO();

			io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
			io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
			io.ConfigWindowsMoveFromTitleBarOnly = true;

			if (true)
			{
				io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
				io.ConfigViewportsNoDefaultParent = true;
			}

			//-------------------------------------------------------------------------

			StringAnsi const outputDir = EngineContext::ProjectCacheFolder.ToStringAnsi();

			StringAnsi const imguiIniPath = outputDir + "SGE_GUI.ini";
			data->m_iniFilename = imguiIniPath;

			StringAnsi const imguiLogPath = outputDir + "SGE_GUI_log.txt";
			data->m_logFilename = imguiLogPath;

			io.IniFilename = data->m_iniFilename.Get();
			io.LogFilename = data->m_logFilename.Get();

			InitializeFonts(io);
			InitializeGraphic(io);
			ImGui_ImplWin32_Init(Window::GetMainWindow()->GetHWND());


			return ISystem::OnInit();
		}

		void OnDispose() override
		{
			ShutdownFonts();
//		ShutdownPlatform();
			ImGui_ImplWin32_Shutdown();

			ImGui::DestroyPlatformWindows();
			ImGui::DestroyContext();
		}

		void InitializeFonts(ImGuiIO &io)
		{
			// Decompress fonts
			//-------------------------------------------------------------------------

			List<uint8> fontData, boldFontData;
			Fonts::GetDecompressedFontData(Fonts::Lexend::Regular::GetData(), fontData);
			Fonts::GetDecompressedFontData(Fonts::Lexend::Bold::GetData(), boldFontData);

			ImWchar const icons_ranges[] = {ICONRANGE_MIN, ICONRANGE_MAX, 0};
			List<uint8> iconFontData;
			Fonts::GetDecompressedFontData((uint8 const *)Fonts::MaterialDesignIcons::GetData(), iconFontData);

			// Base font configs
			//-------------------------------------------------------------------------

			ImFontConfig fontConfig;
			fontConfig.FontDataOwnedByAtlas = false;

			ImFontConfig iconFontConfig;
			iconFontConfig.FontDataOwnedByAtlas = false;
			iconFontConfig.FontBuilderFlags = ImGuiFreeTypeBuilderFlags_LoadColor | ImGuiFreeTypeBuilderFlags_Bitmap;
			iconFontConfig.MergeMode = true;
			iconFontConfig.PixelSnapH = true;
			iconFontConfig.RasterizerMultiply = 1.5f;

			auto CreateFont = [&](List<uint8> &fontData, float fontSize,
				float iconFontSize, GUI::Font fontID, char const *pName, ImVec2 const &glyphOffset)
			{
			  fmt::format_to_n(fontConfig.Name, 40, fontConfig.Name, pName);
			  ImFont *pFont = io.Fonts->AddFontFromMemoryTTF(fontData.Get(), (int32)fontData.Count(), fontSize, &fontConfig);
			  GUI::SystemFonts::s_fonts[(uint8)fontID] = pFont;

			  iconFontConfig.GlyphOffset = glyphOffset;
			  iconFontConfig.GlyphMinAdvanceX = iconFontSize;
			  io.Fonts->AddFontFromMemoryTTF(iconFontData.Get(), (int32)iconFontData.Count(),
				  iconFontSize, &iconFontConfig, icons_ranges);
			};

			constexpr float const DPIScale = 1.0f;
			float const size12 = Math::Floor(12 * DPIScale);
			float const size14 = Math::Floor(14 * DPIScale);
			float const size16 = Math::Floor(16 * DPIScale);
			float const size18 = Math::Floor(18 * DPIScale);
			float const size24 = Math::Floor(24 * DPIScale);

			CreateFont(fontData, size12, size14, GUI::Font::Tiny, "Tiny", ImVec2(0, 2));
			CreateFont(boldFontData, size12, size14, GUI::Font::TinyBold, "Tiny Bold", ImVec2(0, 2));

			CreateFont(fontData, size14, size16, GUI::Font::Small, "Small", ImVec2(0, 2));
			CreateFont(boldFontData, size14, size16, GUI::Font::SmallBold, "Small Bold", ImVec2(0, 2));

			CreateFont(fontData, size16, size18, GUI::Font::Medium, "Medium", ImVec2(0, 2));
			CreateFont(boldFontData, size16, size18, GUI::Font::MediumBold, "Medium Bold", ImVec2(0, 2));

			CreateFont(fontData, size24, size24, GUI::Font::Large, "Large", ImVec2(0, 2));
			CreateFont(boldFontData, size24, size24, GUI::Font::LargeBold, "Large Bold", ImVec2(0, 2));

			// Build font atlas
			//-------------------------------------------------------------------------

			io.Fonts->TexDesiredWidth = 4096;
			io.Fonts->Build();
			ENGINE_ASSERT(io.Fonts->IsBuilt());

			ENGINE_ASSERT(GUI::SystemFonts::s_fonts[(uint8)GUI::Font::Small]->IsLoaded());
			ENGINE_ASSERT(GUI::SystemFonts::s_fonts[(uint8)GUI::Font::SmallBold]->IsLoaded());
			ENGINE_ASSERT(GUI::SystemFonts::s_fonts[(uint8)GUI::Font::Medium]->IsLoaded());
			ENGINE_ASSERT(GUI::SystemFonts::s_fonts[(uint8)GUI::Font::MediumBold]->IsLoaded());
			ENGINE_ASSERT(GUI::SystemFonts::s_fonts[(uint8)GUI::Font::Large]->IsLoaded());
			ENGINE_ASSERT(GUI::SystemFonts::s_fonts[(uint8)GUI::Font::LargeBold]->IsLoaded());

			io.FontDefault = GUI::SystemFonts::s_fonts[(uint8)GUI::Font::Medium];
		}

		void ShutdownFonts()
		{
			for (int i = 0; i < (uint8)GUI::Font::NumFonts; i++)
			{
				GUI::SystemFonts::s_fonts[i] = nullptr;
			}
		}

		void InitializeGraphic(ImGuiIO &io);
	};

	ENGINE_SYSTEM_REGISTER(GUIRendererSystem)


	void GUIRendererSystem::InitializeGraphic(ImGuiIO &io)
	{
		GUIRendererData* t = data;

		data->m_IndexBuffer = New<DynamicIndexBuffer>(10, sizeof(ImDrawIdx), SE_TEXT("GUIIndexBuffer"));
		data->m_VertexBuffer = New<DynamicVertexBuffer>(10, sizeof(ImDrawVert), SE_TEXT("GUIVertexBuffer"));

		GPUDevice* gpuDevice = GPUDevice::instance;
		data->m_FontTexture = gpuDevice->CreateTexture(SE_TEXT("GPUFontTex"));

		unsigned char *pixels;
		int width, height;
		io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

		// Upload texture to graphics system
		GPUTextureDescription textureDesc = GPUTextureDescription::New2D(width, height, 0, PixelFormat::R8G8B8A8_UNorm,
			GPUTextureFlags::ShaderResource, 1);
		textureDesc.MipLevels = 1;
		data->m_FontTexture->Init(textureDesc);
		uint32 row_pitch = 0;
		uint32 slice_pitch = 0;
		data->m_FontTexture->ComputePitch(0, row_pitch, slice_pitch);
		gpuDevice->GetMainContext()->UpdateTexture(data->m_FontTexture, 0, 0, pixels, row_pitch, slice_pitch);
		data->m_FontTexture->SetResidentMipLevels(1);
		io.Fonts->SetTexID((ImTextureID)data->m_FontTexture);

		data->m_Sampler = GPUDevice::instance->CreateSampler();

		GPUSamplerDescription samplerDescription = GPUSamplerDescription::New(GPUSamplerFilter::Bilinear, GPUSamplerAddressMode::Wrap);
		data->m_Sampler->Init(samplerDescription);

		data->m_Shader = AssetContent::LoadAsyncInternal<Shader>(SE_TEXT("Shaders/GUIOld"));
		if (data->m_Shader != nullptr && data->m_Shader->WaitForLoaded())
		{
			GPUPipelineState::Description description = GPUPipelineState::Description::DefaultNoDepth;
			description.BlendMode = BlendingMode::AlphaBlend;
			description.CullMode = CullMode::TwoSided;

			GPUShader* gupShader = data->m_Shader->GetShader();
			description.VS = gupShader->GetVS(SE_TEXT("VS"));
			description.PS = gupShader->GetPS(SE_TEXT("PS"));

			data->m_PipelineState = GPUPipelineState::New();
			data->m_PipelineState->Init(description);
		}
		else
		{
			LOG_FATAL("GUI", "Shader load failed");
		}
	}

    void RenderData(GPUContext * context, ImDrawData const *pDrawData)
    {
        if (!pDrawData || pDrawData->TotalVtxCount == 0)
        {
            return;
        }

        // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
        int fb_width = (int)(pDrawData->DisplaySize.x * pDrawData->FramebufferScale.x);
        int fb_height = (int)(pDrawData->DisplaySize.y * pDrawData->FramebufferScale.y);
        if (fb_width <= 0 || fb_height <= 0)
        {
            return;
        }

		context->SetViewport(fb_width, fb_height);

		// Get memory for vertex and index buffers
		data->m_VertexBuffer->Clear();
		data->m_IndexBuffer->Clear();
		ImDrawVert* vertexBuffer = data->m_VertexBuffer->WriteReserve<ImDrawVert>(pDrawData->TotalVtxCount);
		ImDrawIdx* indexBuffer = data->m_IndexBuffer->WriteReserve<ImDrawIdx>(pDrawData->TotalIdxCount);

		// Copy and convert all vertices into a single contiguous buffer
		for (int cmdListIdx = 0; cmdListIdx < pDrawData->CmdListsCount; cmdListIdx++)
		{
			const ImDrawList *drawList = pDrawData->CmdLists[cmdListIdx];
			Platform::MemoryCopy(vertexBuffer, drawList->VtxBuffer.Data, drawList->VtxBuffer.Size * sizeof(ImDrawVert));
			Platform::MemoryCopy(indexBuffer, drawList->IdxBuffer.Data, drawList->IdxBuffer.Size * sizeof(ImDrawIdx));
			vertexBuffer += drawList->VtxBuffer.Size;
			indexBuffer += drawList->IdxBuffer.Size;
		}

		data->m_VertexBuffer->Flush(context);
		data->m_IndexBuffer->Flush(context);
		// Setup orthographic projection matrix into our constant buffer
		if (data->m_PreDraw == nullptr)
		{
			data->m_PreDraw = context->GetDevice()->CreateConstantBuffer(sizeof(Float4));
		}

        {
            const float L = pDrawData->DisplayPos.x;
            const float R = pDrawData->DisplayPos.x + pDrawData->DisplaySize.x;
            const float T = pDrawData->DisplayPos.y;
            const float B = pDrawData->DisplayPos.y + pDrawData->DisplaySize.y;

			Float4 mvp = Float4(2.0f / (R - L), 2.0f / (T - B),
				(R + L) / (L - R), (T + B) / (B - T));

			context->UpdateCB(data->m_PreDraw, mvp.Raw);
        }

		context->BindVB(data->m_VertexBuffer->GetBuffer());
		context->BindIB(data->m_IndexBuffer->GetBuffer());
		context->BindCB(0, data->m_PreDraw);
		context->BindSampler(0, data->m_Sampler);

        // Will project scissor/clipping rectangles into framebuffer space
        ImVec2 clip_off = pDrawData->DisplayPos;         // (0,0) unless using multi-viewports
        ImVec2 clip_scale = pDrawData->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

        // Render command lists
		int global_vtx_offset = 0;
		int global_idx_offset = 0;
        for (uint32 cmdListIdx = 0; cmdListIdx < (uint32)pDrawData->CmdListsCount; ++cmdListIdx)
        {
            const ImDrawList *drawList = pDrawData->CmdLists[cmdListIdx];
            for (uint32 cmdIndex = 0; cmdIndex < (uint32)drawList->CmdBuffer.size(); ++cmdIndex)
            {
                const ImDrawCmd *drawCmd = &drawList->CmdBuffer[cmdIndex];
                if (drawCmd->UserCallback)
                {
                    // User callback, registered via ImDrawList::AddCallback()
                    // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
                    if (drawCmd->UserCallback == ImDrawCallback_ResetRenderState)
                    {
                    }
                    else
                    {
                        drawCmd->UserCallback(drawList, drawCmd);
                    }
                }
                else
                {
                    // Project scissor/clipping rectangles into framebuffer space
                    ImVec2 clip_min(drawCmd->ClipRect.x - clip_off.x, drawCmd->ClipRect.y - clip_off.y);
                    ImVec2 clip_max(drawCmd->ClipRect.z - clip_off.x, drawCmd->ClipRect.w - clip_off.y);
                    if (clip_max.x < clip_min.x || clip_max.y < clip_min.y)
                        continue;

                    // Apply scissor/clipping rectangle
                    context->SetScissor(Rectangle(clip_min.x, clip_min.y, clip_max.x - clip_min.x, clip_max.y - clip_min.y));

					GPUTexture *texture = (GPUTexture *)drawCmd->TextureId;
					if (texture != nullptr)
					{
						context->BindSR(0, texture);
					}
					context->DrawIndexed(drawCmd->ElemCount, drawCmd->VtxOffset + global_vtx_offset, drawCmd->IdxOffset + global_idx_offset);
                }
            }
			global_idx_offset += drawList->IdxBuffer.Size;
			global_vtx_offset += drawList->VtxBuffer.Size;
        }
    }


	void GUIRenderer::Render(GPUContext * context)
	{
		PROFILE_GPU_CPU("GUI");
		ENGINE_ASSERT(Threading::IsMainThread());

		ImGui::Render();

		GraphicWindow* win = DynamicCast<GraphicWindow>(Window::GetMainWindow());
		GPUSwapChain* swapChain = DynamicCast<GPUSwapChain>(win->GetSwapChain());

		GPUTextureView *backBuffer = swapChain->GetBackBufferView();

		context->SetRenderTarget(backBuffer);
		context->SetState(data->m_PipelineState);
		ImDrawData const *pData = ImGui::GetDrawData();
		// RenderData(context, pData);


		// Viewport Support
		//-------------------------------------------------------------------------
		/*if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();

			//-------------------------------------------------------------------------

			ImGuiPlatformIO &platformIO = ImGui::GetPlatformIO();

			for (int i = 1; i < platformIO.Viewports.Size; i++)
			{
				ImGuiViewport *pViewport = platformIO.Viewports[i];
				if (pViewport->Flags & ImGuiViewportFlags_IsMinimized)
				{
					continue;
				}

                auto pRenderWindow = (RenderWindow *)pViewport->RendererUserData;
                ENGINE_ASSERT(pRenderWindow != nullptr);

                RenderData(context, pViewport->DrawData);
			}
		}#1#
	}

	void GUIRenderer::FrameBegin(float deltaTime)
	{
		ImGuiIO &io = ImGui::GetIO();
		io.DeltaTime = deltaTime;
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
	}*/
}
