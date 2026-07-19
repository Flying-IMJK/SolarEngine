#pragma once

#include "Runtime/Core/Types/Delegate.h"
#include "Editor/CustomEditors/LayoutElement.h"

namespace SE
{
	class ClickableLabel;
}

namespace SE::Editor
{
	class ContextMenu;

	/// <summary>
	/// The label element.
	/// </summary>
	class LabelElement : public LayoutElement
	{
	public:
		/// <summary>
		/// The label.
		/// </summary>
		ClickableLabel* Label;

		/// <summary>
		/// Initializes a new instance of the <see cref="CheckBoxElement"/> class.
		/// </summary>
		LabelElement();

		/// <summary>
		/// Adds a simple context menu with utility to copy label text. Can be extended with more options.
		/// </summary>
		LabelElement* AddCopyContextMenu(Delegate<ContextMenu*> customOptions);

		/// <inheritdoc />
		Control* GetControl() override;

	private:

		Delegate<ContextMenu*> m_CustomContextualOptions;

		void OnRightClick();

		void OnCopyText();
	};

} // SE

