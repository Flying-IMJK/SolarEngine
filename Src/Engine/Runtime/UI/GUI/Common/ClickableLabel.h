#pragma once
#include "Label.h"

namespace SE
{
	/// <summary>
	/// The label that contains events for mouse interaction.
	/// </summary>
	class SE_API_RUNTIME ClickableLabel : public Label
	{
	private:
		bool m_LeftClick = false;
		bool m_IsRightDown = false;

	public:
		/// <summary>
		/// The double click event.
		/// </summary>
		Action DoubleClick;

		/// <summary>
		/// The left mouse button click event.
		/// </summary>
		Action LeftClick;

		/// <summary>
		/// The right mouse button click event.
		/// </summary>
		Action RightClick;

		/// <inheritdoc />
		bool OnMouseDoubleClick(Float2 location, MouseButton button) override;

		/// <inheritdoc />
		bool OnMouseDown(Float2 location, MouseButton button) override;

		/// <inheritdoc />
		bool OnMouseUp(Float2 location, MouseButton button) override;

		/// <inheritdoc />
		void OnMouseLeave() override;
	};

} // SE

