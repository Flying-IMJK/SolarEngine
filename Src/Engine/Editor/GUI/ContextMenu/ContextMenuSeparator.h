#pragma once
#include "ContextMenuItem.h"

namespace SE::Editor
{
	class ContextMenuSeparator : public ContextMenuItem
	{
		SE_CLASS_DEFAULT(ContextMenuSeparator, ContextMenuItem)
	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="ContextMenuSeparator"/> class.
		/// </summary>
		/// <param name="parent">The parent context menu.</param>
		ContextMenuSeparator(ContextMenu* parent);

		/// <inheritdoc />
		void Draw() override;
	};

} // SE

