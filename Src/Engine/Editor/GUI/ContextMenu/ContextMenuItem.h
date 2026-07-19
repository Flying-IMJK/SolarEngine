#pragma once
#include "Runtime/UI/GUI/ContainerControl.h"

namespace SE::Editor
{
	class ContextMenu;

	SE_CLASS(Reflect)
	class ContextMenuItem : public ContainerControl
	{
		SE_DEFINE_CLASS(ContextMenuItem, ContainerControl)
	public:
		/// <summary>
		/// Gets the parent context menu.
		/// </summary>
		ContextMenu* ParentContextMenu;

		/// <summary>
		/// Gets the minimum width of this item.
		/// </summary>
		PRO_GET(MinimumWidth, ContextMenuItem, float, __GetMinimumWidth);


		/// <inheritdoc />
		void OnMouseEnter(Float2 location) override;

		ContextMenuItem();

	protected:
		/// <summary>
		/// Initializes a new instance of the <see cref="ContextMenuItem"/> class.
		/// </summary>
		/// <param name="parent">The parent context menu.</param>
		/// <param name="width">The initial width.</param>
		/// <param name="height">The initial height.</param>
		ContextMenuItem(ContextMenu* parent, float width, float height) : ContainerControl(0, 0, width, height)
		{
			AutoFocus = false;
			ParentContextMenu = parent;
		}

		virtual float __GetMinimumWidth();
	};

} // SE

