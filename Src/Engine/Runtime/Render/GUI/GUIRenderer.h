#pragma once

#include "Runtime/API.h"

struct ImDrawData;
struct ImGuiIO;

namespace SE
{
	class GPUContext;

    //-------------------------------------------------------------------------

    class SE_API_RUNTIME GUIRenderer
    {
    public:
		static void FrameBegin(float deltaTime);

		static void Render(GPUContext* context);
    };
}