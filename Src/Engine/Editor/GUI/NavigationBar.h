#pragma once
#include "Runtime/UI/GUI/Panels/Panel.h"

namespace SE::Editor
{
	class ToolStrip;

	/// <summary>
	/// A navigation bar control. Shows the current location path with UI buttons to navigate around.
	/// </summary>
	class NavigationBar : public Panel
	{
		SE_CLASS(NavigationBar, Panel)
	public:
		/// <summary>
		/// The default buttons margin.
		/// </summary>
		static constexpr float DefaultButtonsMargin = 2;

		NavigationBar();

		/// <summary>
		/// Updates the bar bounds and positions it after the last toolstrip button. Ensures to fit the toolstrip area (navigation bar horizontal scroll bar can be used to view the full path).
		/// </summary>
		/// <param name="toolstrip">The toolstrip.</param>
		void UpdateBounds(ToolStrip* toolstrip);

	protected:
		/// <inheritdoc />
		void Arrange() override;
	};

} // SE

