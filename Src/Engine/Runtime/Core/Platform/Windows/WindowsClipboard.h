#pragma once

#if PLATFORM_WINDOWS

#include "Runtime/Core/Platform/Base/ClipboardBase.h"

namespace SE
{
	/// <summary>
	/// Windows platform implementation of the clipboard service.
	/// </summary>
	class SE_API_RUNTIME WindowsClipboard : public ClipboardBase
	{
	public:
		// [ClipboardBase]
		static void Clear();
		static void SetText(const StringView& text);
		static void SetRawData(const Span<byte>& data);
		static void SetFiles(const List<String>& files);
		static String GetText();
		static List<byte> GetRawData();
		static List<String> GetFiles();
	};

} // SE

#endif

