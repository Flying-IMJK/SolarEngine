#pragma once
#include "ScrollBar.h"

namespace SE
{

	class SE_API_RUNTIME HScrollBar : public ScrollBar
	{
		SE_DEFINE_CLASS(HScrollBar, ScrollBar)
	public:

		HScrollBar();

		/// <summary>
		/// Initializes a new instance of the <see cref="HScrollBar"/> class.
		/// </summary>
		/// <param name="parent">The parent control.</param>
		/// <param name="y">The y position.</param>
		/// <param name="width">The width.</param>
		/// <param name="height">The height.</param>
		HScrollBar(ContainerControl* parent, float y, float width, float height = ScrollBarDefaultSize);

		/// <inheritdoc />
	protected:
		float TrackSize() override;
	};

} // SE

