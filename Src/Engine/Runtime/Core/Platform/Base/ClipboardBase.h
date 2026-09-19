#pragma once

#include <Runtime/Core/Types/Strings/String.h>


namespace SE
{
	class StringView;

	SE_INJECT_CODE(cpp, "#include \"Runtime/Core/Platform/Clipboard.h\"");

	/// <summary>
	/// Native platform clipboard service.
	/// </summary>
	SE_CLASS(API(Static, Name = "Clipboard", Tag = "NativeInvokeUseName"))
	class SE_API_RUNTIME ClipboardBase
	{
		SCRIPTING_TYPE_MIN(ClipboardBase)
	public:
		/// <summary>
		/// Clear the clipboard contents.
		/// </summary>
		SE_FUNCTION(API())
		static void Clear()
		{
		}

		/// <summary>
		/// Sets text to the clipboard.
		/// </summary>
		/// <param name="text">The text to set.</param>
		SE_FUNCTION(API(Prop))
		static void SetText(const StringView& text)
		{
		}

		/// <summary>
		/// Sets the raw bytes data to the clipboard.
		/// </summary>
		/// <param name="data">The data to set.</param>
		SE_FUNCTION(API(Prop))
		static void SetRawData(const Span<byte>& data)
		{
		}

		/// <summary>
		/// Sets the files to the clipboard.
		/// </summary>
		/// <param name="files">The list of file paths.</param>
		SE_FUNCTION(API(Prop))
		static void SetFiles(const List<String>& files)
		{
		}

		/// <summary>
		/// Gets the text from the clipboard.
		/// </summary>
		/// <returns>The result text (or empty if clipboard doesn't have valid data).</returns>
		SE_FUNCTION(API(Prop))
		static String GetText()
		{
			return String::Empty;
		}

		/// <summary>
		/// Gets the raw bytes data from the clipboard.
		/// </summary>
		/// <returns>The result data (or empty if clipboard doesn't have valid data).</returns>
		SE_FUNCTION(API(Prop))
		static List<byte> GetRawData()
		{
			return List<byte>();
		}

		/// <summary>
		/// Gets the file paths from the clipboard.
		/// </summary>
		/// <returns>The output list of file paths (or empty if clipboard doesn't have valid data).</returns>
		SE_FUNCTION(API(Prop))
		static List<String> GetFiles()
		{
			return List<String>();
		}
	};
}
