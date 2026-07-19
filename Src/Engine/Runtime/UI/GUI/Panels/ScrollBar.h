#pragma once

#include "Runtime/UI/GUI/Control.h"

namespace SE
{
	#define ScrollBarDefaultSize 14

	SE_CLASS(Reflect)
	class SE_API_RUNTIME ScrollBar : public Control
	{
		SE_DEFINE_CLASS(ScrollBar, Control)
	private:

	    // Scrolling
	    float _clickChange = 20, _scrollChange = 50;
	    float _minimum, _maximum = 100;
	    float _startValue, _value, _targetValue;
	    Orientation _orientation;
	    Delegate<float> _update;
		Delegate<float> m_OnUpdate;

	    // Input

	    float _mouseOffset;

	    // Thumb data

	    Rectangle _thumbRect, _trackRect;
	    bool _thumbClicked;
	    float _thumbCenter, _thumbSize;

	    // Smoothing

	    float _thumbOpacity = DefaultMinimumOpacity;
	    float _scrollAnimationProgress = 0.0f;

	public:
        /// <summary>
        /// The default minimum opacity.
        /// </summary>
        const float DefaultMinimumOpacity = 0.75f;


        /// <summary>
        /// Gets the orientation.
        /// </summary>
        Orientation GetOrientation() const { return _orientation; };

        /// <summary>
        /// Gets or sets the thumb box thickness.
        /// </summary>
        float ThumbThickness = 8;

        /// <summary>
        /// Gets or sets the track line thickness.
        /// </summary>
        float TrackThickness = 2.0f;

        /// <summary>
        /// The maximum time it takes to animate from current to target scroll position
        /// </summary>
        float ScrollAnimationDuration = 0.18f;

        /// <summary>
        /// Gets a value indicating whether use scroll value smoothing.
        /// </summary>
        bool UseSmoothing() const;

        /// <summary>
        /// Enables scroll smoothing
        /// </summary>
        bool EnableSmoothing = true;

        /// <summary>
        /// The track color.
        /// </summary>
        Color TrackColor;

        /// <summary>
        /// The thumb color.
        /// </summary>
        Color ThumbColor;

        /// <summary>
        /// The selected thumb color.
        /// </summary>
        Color ThumbSelectedColor;

        /// <summary>
        /// Gets or sets the minimum value.
        /// </summary>
	    float GetMinimum() const { return _minimum; };
        void SetMinimum(float value);

        /// <summary>
        /// Gets or sets the maximum value.
        /// </summary>
	    float GetMaximum() const { return _maximum; };
        void SetMaximum(float value);

        /// <summary>
        /// Gets or sets the scroll value (current, smooth).
        /// </summary>
        float GetValue() const { return _value; }
	    void SetValue(float value);

        /// <summary>
        /// Gets or sets the target value (target, not smooth).
        /// </summary>
        float GetTargetValue() const { return _targetValue; }
		void SetTargetValue(float value);

        /// <summary>
        /// Gets or sets the speed for the scroll on mouse wheel.
        /// </summary>
        float ScrollSpeedWheel;

        /// <summary>
        /// Gets or sets the speed for the scroll on mouse click.
        /// </summary>
        float ScrollSpeedClick;

        /// <summary>
        /// Gets the value slow down.
        /// </summary>
        float ValueSlowDown() const { return _targetValue - _value;}

        /// <summary>
        /// Gets a value indicating whether thumb is being clicked (scroll bar is in use).
        /// </summary>
        bool IsThumbClicked() const { return _thumbClicked; }

        /// <summary>
        /// Occurs when value gets changed.
        /// </summary>
	    Action ValueChanged;

        /// <summary>
        /// Enables/disabled scrolling by user.
        /// </summary>
        bool ThumbEnabled = true;

        /// <summary>
        /// Cuts the scroll bar value smoothing and imminently goes to the target scroll value.
        /// </summary>
        void FastScroll();

        /// <summary>
        /// Scrolls the view to the desire range (favors minimum value if cannot cover whole range in a bounds).
        /// </summary>
        /// <param name="min">The view minimum.</param>
        /// <param name="max">The view maximum.</param>
        /// <param name="fastScroll">True of scroll to the item quickly without smoothing.</param>
        void ScrollViewTo(float min, float max, bool fastScroll = false);

        /// <summary>
        /// Sets the scroll range (min and max at once).
        /// </summary>
        /// <param name="minimum">The minimum scroll range value (see <see cref="Minimum"/>).</param>
        /// <param name="maximum">The maximum scroll range value (see <see cref="Minimum"/>).</param>
        void SetScrollRange(float minimum, float maximum);

        /// <inheritdoc />
		void Draw() override;

        /// <inheritdoc />
		void OnLostFocus() override;

        /// <inheritdoc />
		void OnMouseMove(Float2 location) override;

        /// <inheritdoc />
		bool OnMouseWheel(Float2 location, float delta) override;

        /// <inheritdoc />
        bool OnMouseDown(Float2 location, MouseButton button) override;

        /// <inheritdoc />
        bool OnMouseUp(Float2 location, MouseButton button) override;

        /// <inheritdoc />
        void OnEndMouseCapture() override;

        /// <inheritdoc />
        void OnMouseEnter(Float2 location) override;

        /// <inheritdoc />
	    void OnMouseLeave() override;

		void Reset();

	private:
		void UpdateThumb();

		void EndTracking();

		void OnUpdate(float deltaTime);

	protected:
        /// <summary>
        /// Called when value gets changed.
        /// </summary>
        virtual void OnValueChanged();

	    /// <inheritdoc />
	    void OnSizeChanged() override;

	protected:

	    /// <summary>
	    /// Gets the size of the track.
	    /// </summary>
	    virtual float TrackSize() = 0;

		ScrollBar();

	    /// <summary>
	    /// Initializes a new instance of the <see cref="ScrollBar"/> class.
	    /// </summary>
	    /// <param name="orientation">The orientation.</param>
	    explicit ScrollBar(Orientation orientation);

	    /// <inheritdoc />
	    void AddUpdateCallbacks(RootControl* root) override;

        /// <inheritdoc />
	    void RemoveUpdateCallbacks(RootControl* root) override;

	};

} // SE

