#pragma once

#include "ContainerControl.h"

namespace SE
{
    class GraphicWindow;
    class Control;

    class SE_API_RUNTIME Tooltip : public ContainerControl
    {
        SE_DEFINE_CLASS(Tooltip, ContainerControl)
    private:
        float m_TimeToPopupLeft;
        Control* m_LastTarget;
        Control* m_ShowTarget;
        String m_CurrentText;
        GraphicWindow* m_Window;

        void UpdateWindowSize();

        void WrapPosition(Float2 locationSS, float flipOffset = 0.0f);

    public:
        /// <summary>
        /// Gets or sets the time in seconds that mouse have to be over the target to show the tooltip.
        /// </summary>
        float TimeToShow = 0.3f; // 300ms by default

        /// <summary>
        /// Gets or sets the maximum width of the tooltip. Used to wrap text that overflows and ensure that tooltip stays readable.
        /// </summary>
        float MaxWidth = 500.0f;

        /// <summary>
        /// Gets the tooltip window.
        /// </summary>
        GraphicWindow* Window() const { return m_Window; }

        /// <summary>
        /// Shows tooltip over given control.
        /// </summary>
        /// <param name="target">The parent control to attach to it.</param>
        /// <param name="location">Popup menu origin location in parent control coordinates.</param>
        /// <param name="targetArea">Tooltip target area of interest.</param>
        void Show(Control* target, Float2 location, Rectangle targetArea);

        /// <summary>
        /// Hides the popup.
        /// </summary>
        void Hide();

        /// <summary>
        /// Called when mouse enters a control.
        /// </summary>
        /// <param name="target">The target.</param>
        void OnMouseEnterControl(Control* target);

        /// <summary>
        /// Called when mouse is over a control.
        /// </summary>
        /// <param name="target">The target.</param>
        /// <param name="dt">The delta time.</param>
        void OnMouseOverControl(Control* target, float dt);

        /// <summary>
        /// Called when mouse leaves a control.
        /// </summary>
        /// <param name="target">The target.</param>
        void OnMouseLeaveControl(Control* target);

        /// <inheritdoc />
        void Update(float deltaTime) override;

        /// <inheritdoc />
        void Draw() override;

        /// <inheritdoc />
        bool OnShowTooltip(String text, Float2& location, Rectangle& area) override;

        /// <inheritdoc />
        void OnDestroy() override;

        Tooltip();

    protected:
        /// <inheritdoc />
        void PerformLayoutBeforeChildren() override;
    };
} // SE

