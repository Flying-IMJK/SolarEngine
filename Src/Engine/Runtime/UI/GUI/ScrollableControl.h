#pragma once

#include "ContainerControl.h"

namespace SE
{
	class SE_API_RUNTIME ScrollableControl : public ContainerControl
	{
		SE_CLASS(ScrollableControl, ContainerControl)
protected:
        /// <summary>
        /// The view offset. Useful to offset contents of the container (used by the scrollbars and drop panels).
        /// </summary>
        Float2 _viewOffset;


	    /// <summary>
	    /// Called when view offset gets changed.
	    /// </summary>
		virtual void OnViewOffsetChanged()
	    {
	    }

	    /// <inheritdoc />
		void DrawChildren() override;

public:
		ScrollableControl() = default;

        /// <summary>
        /// Gets current view offset for all the controls (used by the scroll bars).
        /// </summary>
        Float2 GetViewOffset() const { return _viewOffset; }
		/// <summary>
		/// Sets the view offset.
		/// </summary>
		/// <param name="value">The value.</param>
		virtual void SetViewOffset(Float2 value)
        {
        	_viewOffset = value;
        	OnViewOffsetChanged();
        }

        /// <inheritdoc />
		bool IntersectsChildContent(Control* child, Float2 location, Float2& childSpaceLocation) override;

        /// <inheritdoc />
		bool IntersectsContent(Float2& locationParent, Float2& location) override;

        /// <inheritdoc />
		Float2 PointToParent(Float2 location) override;

        /// <inheritdoc />
		Float2 PointFromParent(Float2 location) override;
	};

} // SE

