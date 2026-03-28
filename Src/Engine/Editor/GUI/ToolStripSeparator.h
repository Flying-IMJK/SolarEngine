#pragma once
#include "Runtime/UI/GUI/Control.h"

namespace SE::Editor
{
	/// <summary>
	/// ToolStrip separator control.
	/// </summary>
	class ToolStripSeparator : public Control
	{
		SE_CLASS_DEFAULT(ToolStripSeparator, Control)
	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="ToolStripSeparator"/> class.
		/// </summary>
		/// <param name="height">The height.</param>
		ToolStripSeparator(float height);

		/// <inheritdoc />
		void Draw() override;
	};
} // SE

