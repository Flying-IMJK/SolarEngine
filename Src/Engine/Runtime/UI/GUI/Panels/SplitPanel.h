#pragma once

#include "Runtime/UI/GUI/ContainerControl.h"

namespace SE
{
    class Panel;

    // <summary>
    /// GUI control that contains two child panels and the splitter between them.
    /// </summary>
    class SE_API_RUNTIME SplitPanel : public ContainerControl
    {
    private:
        Orientation _orientation;
        float _splitterValue;
        Rectangle _splitterRect;
        bool _splitterClicked, _mouseOverSplitter;
        bool _cursorChanged;

        void UpdateSplitRect();

        void StartTracking();

        void EndTracking();
        
    public:

        /// <summary>
        /// The splitter size (in pixels).
        /// </summary>
        constexpr static int SplitterSize = 4;

        /// <summary>
        /// The splitter half size (in pixels).
        /// </summary>
        constexpr static int SplitterSizeHalf = SplitterSize / 2;
        
        /// <summary>
        /// The first panel (left or upper based on Orientation).
        /// </summary>
        Panel* Panel1;

        /// <summary>
        /// The second panel.
        /// </summary>
        Panel* Panel2;

        /// <summary>
        /// Gets or sets the panel orientation.
        /// </summary>
        /// <value>
        /// The orientation.
        /// </value>
        PRO(Orientation, SplitPanel, Orientation, __GetOrientation, __SetOrientation);

        /// <summary>
        /// Gets or sets the splitter value (always in range [0; 1]).
        /// </summary>
        /// <value>
        /// The splitter value (always in range [0; 1]).
        /// </value>
        PRO(SplitterValue, SplitPanel, float, __GetSplitterValue, __SetSplitterValue);


        /// <summary>
        /// Initializes a new instance of the <see cref="SplitPanel"/> class.
        /// </summary>
        /// <param name="orientation">The orientation.</param>
        /// <param name="panel1Scroll">The panel1 scroll bars.</param>
        /// <param name="panel2Scroll">The panel2 scroll bars.</param>
        SplitPanel(::SE::Orientation orientation = Orientation::Horizontal, ScrollBars panel1Scroll = ScrollBars::Both, ScrollBars panel2Scroll = ScrollBars::Both);


        /// <inheritdoc />
        void Draw() override;

        /// <inheritdoc />
        void OnLostFocus() override;

        /// <inheritdoc />
        void OnMouseMove(Float2 location) override;

        /// <inheritdoc />
        bool OnMouseDown(Float2 location, MouseButton button) override;

        /// <inheritdoc />
        bool OnMouseUp(Float2 location, MouseButton button) override;

        /// <inheritdoc />
        void OnMouseLeave() override;

        /// <inheritdoc />
        void OnEndMouseCapture() override;

    protected:
        /// <inheritdoc />
        void OnSizeChanged() override;

        /// <inheritdoc />
        void PerformLayoutBeforeChildren() override;

    private:
        ::SE::Orientation __GetOrientation() { return _orientation;}
        void __SetOrientation(::SE::Orientation value);

        float __GetSplitterValue() { return _splitterValue; }
        void __SetSplitterValue(float value);
    };
} // SE
