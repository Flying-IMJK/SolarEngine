#pragma once

#include "Runtime/API.h"
#include "Core/Types/Variable.h"
#include "Core/Types/Collections/List.h"
//-------------------------------------------------------------------------

namespace SE::Fonts
{
	SE_API_RUNTIME void GetDecompressedFontData(uint8 const* pSourceData, List<uint8>& fontData);

	namespace Proggy::Tiny
	{
		SE_API_RUNTIME uint8 const* GetData();
	}

	namespace Proggy::Clean
	{
		SE_API_RUNTIME uint8 const* GetData();
	}

	namespace Lexend::Regular
	{
		SE_API_RUNTIME uint8 const* GetData();
	}

	namespace Lexend::Medium
	{
		SE_API_RUNTIME uint8 const* GetData();
	}

	namespace Lexend::Bold
	{
		SE_API_RUNTIME uint8 const* GetData();
	}

	namespace MaterialDesignIcons
	{
		uint8 SE_API_RUNTIME const* GetData();
	}
}

