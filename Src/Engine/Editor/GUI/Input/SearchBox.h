#pragma once

#include "Runtime/UI/GUI/Common/TextBox.h"

namespace SE
{
	class Button;
}

namespace SE::Editor
{
	/// <summary>
	/// Search box control which can gather text search input from the user.
	/// </summary>
	class SearchBox : public TextBox
	{
		SE_DEFINE_CLASS(SearchBox, TextBox)
	public:
		/// <summary>
		/// A button that clears the search bar.
		/// </summary>
		Button* ClearSearchButton;

		SearchBox();

		/// <summary>
		/// Init search box
		/// </summary>
		SearchBox(bool isMultiline, float x, float y, float width = 120);
	};

} // SE
