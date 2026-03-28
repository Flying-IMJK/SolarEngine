#pragma once

#include "Core/Types/Strings/String.h"

namespace SE
{
	class StringView;

	/// <summary>
	/// Native platform clipboard service.
	/// </summary>
	class SE_API_CORE ClipboardBase
	{
	public:
		/// <summary>
		/// Clear the clipboard contents.
		/// </summary>
		static void Clear()
		{
		}

		/// <summary>
		/// Sets text to the clipboard.
		/// </summary>
		/// <param name="text">The text to set.</param>
		static void SetText(const StringView& text)
		{
		}

		/// <summary>
		/// Sets the raw bytes data to the clipboard.
		/// </summary>
		/// <param name="data">The data to set.</param>
		static void SetRawData(const Span<byte>& data)
		{
		}

		/// <summary>
		/// Sets the files to the clipboard.
		/// </summary>
		/// <param name="files">The list of file paths.</param>
		static void SetFiles(const List<String>& files)
		{
		}

		/// <summary>
		/// Gets the text from the clipboard.
		/// </summary>
		/// <returns>The result text (or empty if clipboard doesn't have valid data).</returns>
		static String GetText()
		{
			return String::Empty;
		}

		/// <summary>
		/// Gets the raw bytes data from the clipboard.
		/// </summary>
		/// <returns>The result data (or empty if clipboard doesn't have valid data).</returns>
		static List<byte> GetRawData()
		{
			return List<byte>();
		}

		/// <summary>
		/// Gets the file paths from the clipboard.
		/// </summary>
		/// <returns>The output list of file paths (or empty if clipboard doesn't have valid data).</returns>
		static List<String> GetFiles()
		{
			return List<String>();
		}
	};
}
