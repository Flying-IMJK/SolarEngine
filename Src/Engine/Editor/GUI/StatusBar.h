#pragma once
#include "Runtime/UI/GUI/ContainerControl.h"
#include "Runtime/UI/GUI/Style.h"

namespace SE::Editor
{

	/// <summary>
	/// Status strip GUI control.
	/// </summary>
	/// <seealso cref="FlaxEngine.GUI.ContainerControl" />
	class StatusBar : public ContainerControl
	{
	public:
		/// <summary>
		/// The default height.
		/// </summary>
		constexpr static int DefaultHeight = 22;

		/// <summary>
		/// Gets or sets the color of the status strip.
		/// </summary>
		PRO_REF(StatusColor, StatusBar, Color, __GetStatusColor, __SetStatusColor);

		/// <summary>
		/// Gets or sets the status text.
		/// </summary>
		String Text;

		/// <summary>
		/// Gets or sets the status text color
		/// </summary>
		Color TextColor = Style::Current->Foreground;

		/// <summary>
		/// Initializes a new instance of the <see cref="StatusBar"/> class.
		/// </summary>
		StatusBar();

		/// <inheritdoc />
		void Draw() override;

	private:
		Color& __GetStatusColor() { return BackgroundColor; }
		void __SetStatusColor(Color &value) { BackgroundColor = value; }
	};

} // SE