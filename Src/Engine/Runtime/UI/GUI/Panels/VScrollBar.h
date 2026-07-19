#pragma once
#include "ScrollBar.h"

namespace SE
{

	SE_CLASS(Reflect)
	class SE_API_RUNTIME VScrollBar : public ScrollBar
	{
		SE_DEFINE_CLASS(VScrollBar, ScrollBar)
	public:

		VScrollBar();

		/// <summary>
		/// Initializes a new instance of the <see cref="VScrollBar"/> class.
		/// </summary>
		/// <param name="parent">The parent control.</param>
		/// <param name="x">The x position.</param>
		/// <param name="height">The height.</param>
		/// <param name="width">The width.</param>
		VScrollBar(ContainerControl* parent, float x, float height, float width = ScrollBarDefaultSize);

	protected:
		/// <inheritdoc />
		 float TrackSize() override;
	};

} // SE

