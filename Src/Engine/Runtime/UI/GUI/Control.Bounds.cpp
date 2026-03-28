#include "ContainerControl.h"
#include "Control.h"


namespace SE
{
	void Control::__SetCenter(Float2 value)
	{
		Float2 t = value - __GetSize() * 0.5f;
		__SetLocation(t);
	}

	void Control::__SetBounds(Rectangle &value)
	{
		if (m_Bounds == value)
		{
			return;
		}

		// Calculate anchors based on the parent container client area
		Margin anchors;
		if (m_Parent != nullptr)
		{
			Rectangle parentBounds = m_Parent->GetDesireClientArea();
			anchors = Margin
			(
			 m_AnchorMin.x * parentBounds.Size.x + parentBounds.Location.x,
			 m_AnchorMax.x * parentBounds.Size.x,
			 m_AnchorMin.y * parentBounds.Size.y + parentBounds.Location.y,
			 m_AnchorMax.y * parentBounds.Size.y
			);
		}
		else
		{
			anchors = Margin::Zero;
		}

		// Calculate offsets on X axis
		m_Offsets.Left = value.Location.x - anchors.Left;
		if (m_AnchorMin.x != m_AnchorMax.x)
		{
			m_Offsets.Right = anchors.Right - value.Location.x - value.Size.x;
		}
		else
		{
			m_Offsets.Right = value.Size.x;
		}

		// Calculate offsets on Y axis
		m_Offsets.Top = value.Location.y - anchors.Top;
		if (m_AnchorMin.y != m_AnchorMax.y)
		{
			m_Offsets.Bottom = anchors.Bottom - value.Location.y - value.Size.y;
		}
		else
		{
			m_Offsets.Bottom = value.Size.y;
		}

		// Flush the control bounds
		UpdateBounds();
	}

	void Control::__SetScale(Float2& value)
	{
		if (m_Scale != value)
		{
			SetScaleInternal(value);
		}
	}

	void Control::__SetPivot(Float2& value)
	{
		if (m_Pivot != value)
		{
			SetPivotInternal(value);
		}
	}

	void Control::__SetShear(Float2& value)
	{
		if (m_Shear != value)
		{
			SetShearInternal(value);
		}
	}

	void Control::__SetRotation(float value)
	{
		if (!Math::IsNearEqual(m_Rotation, value))
		{
			SetRotationInternal(value);
		}
	}

	void Control::UpdateBounds()
	{
		auto prevBounds = m_Bounds;

		// Calculate anchors based on the parent container client area
		Margin anchors;
		Float2 offset;
		if (m_Parent != nullptr)
		{
			Rectangle parentBounds = m_Parent->GetDesireClientArea();
			anchors = Margin
			(
			 m_AnchorMin.x * parentBounds.Size.x,
			 m_AnchorMax.x * parentBounds.Size.x,
			 m_AnchorMin.y * parentBounds.Size.y,
			 m_AnchorMax.y * parentBounds.Size.y
			);
			offset = parentBounds.Location;
		}
		else
		{
			anchors = Margin::Zero;
			offset = Float2::Zero;
		}

		// Calculate position and size on X axis
		m_Bounds.Location.x = anchors.Left + m_Offsets.Left;
		if (m_AnchorMin.x != m_AnchorMax.x)
		{
			m_Bounds.Size.x = anchors.Right - m_Bounds.Location.x - m_Offsets.Right;
		}
		else
		{
			m_Bounds.Size.x = m_Offsets.Right;
		}

		// Calculate position and size on Y axis
		m_Bounds.Location.y = anchors.Top + m_Offsets.Top;
		if (m_AnchorMin.y != m_AnchorMax.y)
		{
			m_Bounds.Size.y = anchors.Bottom - m_Bounds.Location.y - m_Offsets.Bottom;
		}
		else
		{
			m_Bounds.Size.y = m_Offsets.Bottom;
		}

		// Apply the offset
		m_Bounds.Location += offset;

		// Update cached transformation matrix
		UpdateTransform();

		// Handle location/size changes
		if (m_Bounds.Location != prevBounds.Location)
		{
			OnLocationChanged();
		}

		if (m_Bounds.Size != prevBounds.Size)
		{
			OnSizeChanged();
		}
	}

	void Control::UpdateTransform()
	{
		// Actual pivot and negative pivot
		//Float2.Multiply(ref _pivot, ref _bounds.Size, out var v1);
		//Float2.Negate(ref v1, out var v2);
		//Float2.Add(ref v1, ref _bounds.Location, out v1);
		auto v1 = m_Pivot * m_Bounds.Size;
		auto v2 = -v1;
		v1 += m_Bounds.Location;

		// ------ Matrix3x3 based version:

		/*
		// Negative pivot
		Matrix3x3 m1, m2;
		Matrix3x3.Translation2D(ref v2, out m1);

		// Scale
		Matrix3x3.Scaling(_scale.x, _scale.y, 1, out m2);
		Matrix3x3.Multiply(ref m1, ref m2, out m1);

		// Shear
		Matrix3x3.Shear(ref _shear, out m2);
		Matrix3x3.Multiply(ref m1, ref m2, out m1);

		// Rotation
		Matrix3x3.RotationZ(_rotation * Mathf.DegreesToRadians, out m2);
		Matrix3x3.Multiply(ref m1, ref m2, out m1);

		// Pivot + Location
		Matrix3x3.Translation2D(ref v1, out m2);
		Matrix3x3.Multiply(ref m1, ref m2, out _cachedTransform);
		*/

		// ------ Matrix2x2 based version:

		// 2D transformation

		//Matrix2x2.Scale(ref _scale, out Matrix2x2 m1);
		//Matrix2x2.Shear(ref _shear, out Matrix2x2 m2);
		//Matrix2x2.Multiply(ref m1, ref m2, out m1);

		// Scale and Shear
		Matrix3x3 m1 = Matrix3x3
		(
			m_Scale.x,
			m_Scale.x * (m_Shear.y == 0 ? 0 : (1.0f / Math::Tan(Math::DegreesToRadians * (90 - Math::Clamp(m_Shear.y, -89.0f, 89.0f))))),
			0,
			m_Scale.y * (m_Shear.x == 0 ? 0 : (1.0f / Math::Tan(Math::DegreesToRadians * (90 - Math::Clamp(m_Shear.x, -89.0f, 89.0f))))),
			m_Scale.y,
			0, 0, 0, 1
		);


		//Matrix2x2.Rotation(Mathf.DegreesToRadians * _rotation, out m2);
		float sin = Math::Sin(Math::DegreesToRadians * m_Rotation);
		float cos = Math::Cos(Math::DegreesToRadians * m_Rotation);

		//Matrix2x2.Multiply(ref m1, ref m2, out m1);
		m1.M11 = (m_Scale.x * cos) + (m1.M12 * -sin);
		m1.M12 = (m_Scale.x * sin) + (m1.M12 * cos);
		float m21 = (m1.M21 * cos) + (m_Scale.y * -sin);
		m1.M22 = (m1.M21 * sin) + (m_Scale.y * cos);
		m1.M21 = m21;
		// Mix all the stuff
		//Matrix3x3.Translation2D(ref v2, out Matrix3x3 m3);
		//Matrix3x3 m4 = (Matrix3x3)m1;
		//Matrix3x3.Multiply(ref m3, ref m4, out m3);
		//Matrix3x3.Translation2D(ref v1, out m4);
		//Matrix3x3.Multiply(ref m3, ref m4, out _cachedTransform);
		m1.M31 = (v2.x * m1.M11) + (v2.y * m1.M21) + v1.x;
		m1.M32 = (v2.x * m1.M12) + (v2.y * m1.M22) + v1.y;
		m_CachedTransform = m1;

		// Cache inverted transform
		Matrix3x3::Invert(m_CachedTransform, m_CachedTransformInv);
	}

	void Control::__SetName(String& value)
	{
		m_Name = value;
	}

	float Control::__GetX()
	{
		return m_Bounds.GetX();
	}

	void Control::__SetX(float value)
	{
		Bounds = Rectangle(value, Y, m_Bounds.Size.x, m_Bounds.Size.y);
	}

	float Control::__GetY()
	{
		return m_Bounds.GetY();
	}

	void Control::__SetY(float value)
	{
		Bounds = Rectangle(X, value, m_Bounds.Size.x, m_Bounds.Size.y);
	}

	float Control::__GetLocalX()
	{
		return __GetLocalLocation().x;
	}

	void Control::__SetLocalX(float value)
	{
		Float2 t = Float2(value, __GetLocalLocation().y);
		__SetLocalLocation(t);
	}

	float Control::__GetLocalY()
	{
		return __GetLocalLocation().y;
	}

	void Control::__SetLocalY(float value)
	{
		__SetLocalLocation({LocalLocation.Get().x, value});
	}

	Float2& Control::__GetAnchorMin()
	{
		return m_AnchorMin;
	}

	void Control::__SetAnchorMin(Float2& value)
	{
		if (m_AnchorMin != value)
		{
			Rectangle bounds = m_Bounds;
			m_AnchorMin = value;
			UpdateBounds();
			Bounds = bounds;
		}
	}

	Float2& Control::__GetAnchorMax()
	{
		return m_AnchorMax;
	}

	void Control::__SetAnchorMax(Float2 &value)
	{
		if (m_AnchorMax != value)
		{
			Rectangle bounds = m_Bounds;
			m_AnchorMax = value;
			UpdateBounds();
			Bounds = bounds;
		}
	}

	Margin& Control::__GetOffsets()
	{
		return m_Offsets;
	}

	void Control::__SetOffsets(Margin &value)
	{
		if (m_Offsets != value)
		{
			m_Offsets = value;
			UpdateBounds();
		}
	}

	Float2 Control::__GetLocation()
	{
		return LocalLocation;
	}

	void Control::__SetLocation(Float2 value)
	{
		if (m_Bounds.Location == value)
			return;
		Bounds = Rectangle(value, m_Bounds.Size);
	}

	Float2 Control::__GetLocalLocation()
	{
		Float2 localLocation = m_Bounds.Location;
		if (m_Parent != nullptr)
		{
			localLocation - (m_Parent->m_Bounds.Size * (m_AnchorMax + m_AnchorMin) * 0.5f);
		}
		localLocation += m_Bounds.Size * m_Pivot;
		return localLocation;
	}

	void Control::__SetLocalLocation(Float2 value)
	{
		if (m_Parent != nullptr)
		{
			Bounds = Rectangle(value + (m_Parent->Bounds.Get().Size * (m_AnchorMax + m_AnchorMin) * 0.5f) - m_Bounds.Size * m_Pivot, m_Bounds.Size);
		}
		else
		{
			Bounds = Rectangle(value - m_Bounds.Size * m_Pivot, m_Bounds.Size);
		}
	}

	bool Control::__GetPivotRelative()
	{
		return _pivotRelativeSizing;
	}

	void Control::__SetPivotRelative(bool value)
	{
		_pivotRelativeSizing = value;
	}

	float Control::__GetWidth()
	{
		return m_Bounds.Size.x;
	}

	void Control::__SetWidth(float value)
	{
		if (Math::IsNearEqual(m_Bounds.Size.x, value))
		{
			return;
		}
		Rectangle bounds = Rectangle(m_Bounds.Location.x, m_Bounds.Location.y, value, m_Bounds.Size.y);
		if (_pivotRelativeSizing)
		{
			float delta = m_Bounds.Size.x - value;
			bounds.Location.x += delta * Pivot.Get().x;
		}
		Bounds = bounds;
	}

	float Control::__GetHeight()
	{
		return m_Bounds.Size.y;
	}

	void Control::__SetHeight(float value)
	{
		if (Math::IsNearEqual(m_Bounds.Size.y, value))
		{
			return;
		}
		Rectangle bounds = Rectangle(m_Bounds.Location.x, m_Bounds.Location.y, m_Bounds.Size.x, value);
		if (_pivotRelativeSizing)
		{
			float delta = m_Bounds.Size.y - value;
			bounds.Location.y += delta * Pivot.Get().y;
		}
		Bounds = bounds;
	}

	Float2& Control::__GetSize()
	{
		return m_Bounds.Size;
	}

	void Control::__SetSize(Float2& value)
	{
		if (m_Bounds.Size == value)
			return;

		Rectangle rectangle = Rectangle(m_Bounds.Location, value);
		__SetBounds(rectangle);
	}

	float Control::__GetTop()
	{
		return m_Bounds.GetTop();
	}

	float Control::__GetBottom()
	{
		return m_Bounds.GetBottom();
	}

	float Control::__GetLeft()
	{
		return m_Bounds.GetLeft();
	}

	float Control::__GetRight()
	{
		return m_Bounds.GetRight();
	}

	Float2 Control::__GetUpperLeft()
	{
		return m_Bounds.GetUpperLeft();
	}

	Float2 Control::__GetUpperRight()
	{
		return m_Bounds.GetUpperRight();
	}

	Float2 Control::__GetBottomRight()
	{
		return m_Bounds.GetBottomRight();
	}

	Float2 Control::__GetBottomLeft()
	{
		return m_Bounds.GetBottomLeft();
	}

	Float2 Control::__GetCenter()
	{
		return m_Bounds.GetCenter();
	}

	void Control::SetAnchorPreset(AnchorPresets anchorPreset, bool preserveBounds, bool setPivotToo)
	{
		for (int i = 0; i < ARRAY_SIZE(anchorPresetsData); i++)
		{
			if (anchorPresetsData[i].Preset == anchorPreset)
			{
				auto anchorMin = anchorPresetsData[i].Min;
				auto anchorMax = anchorPresetsData[i].Max;
				auto bounds = m_Bounds;
				if (!Float2::NearEqual(m_AnchorMin, anchorMin) ||
					!Float2::NearEqual(m_AnchorMax, anchorMax))
				{
					// Disable scrolling for anchored controls (by default but can be manually restored)
					if (!anchorMin.IsZero() || !anchorMax.IsZero())
						IsScrollable = false;

					m_AnchorMin = anchorMin;
					m_AnchorMax = anchorMax;
					if (preserveBounds)
					{
						UpdateBounds();
						Bounds = bounds;
					}
				}
				if (!preserveBounds)
				{
					if (m_Parent != nullptr)
					{
						auto parentBounds = m_Parent->GetDesireClientArea();
						switch (anchorPreset)
						{
						case AnchorPresets::TopLeft:
							bounds.Location = Float2::Zero;
							break;
						case AnchorPresets::TopCenter:
							bounds.Location = Float2(parentBounds.GetLeft() * 0.5f - bounds.GetLeft() * 0.5f, 0);
							break;
						case AnchorPresets::TopRight:
							bounds.Location = Float2(parentBounds.GetLeft() - bounds.GetLeft(), 0);
							break;
						case AnchorPresets::MiddleLeft:
							bounds.Location = Float2(0, parentBounds.GetHeight() * 0.5f - bounds.GetHeight() * 0.5f);
							break;
						case AnchorPresets::MiddleCenter:
							bounds.Location = Float2(parentBounds.GetLeft() * 0.5f - bounds.GetLeft() * 0.5f, parentBounds.GetHeight() * 0.5f - bounds.GetHeight() * 0.5f);
							break;
						case AnchorPresets::MiddleRight:
							bounds.Location = Float2(parentBounds.GetLeft() - bounds.GetLeft(), parentBounds.GetHeight() * 0.5f - bounds.GetHeight() * 0.5f);
							break;
						case AnchorPresets::BottomLeft:
							bounds.Location = Float2(0, parentBounds.GetHeight() - bounds.GetHeight());
							break;
						case AnchorPresets::BottomCenter:
							bounds.Location = Float2(parentBounds.GetLeft() * 0.5f - bounds.GetLeft() * 0.5f, parentBounds.GetHeight() - bounds.GetHeight());
							break;
						case AnchorPresets::BottomRight:
							bounds.Location = Float2(parentBounds.GetLeft() - bounds.GetLeft(), parentBounds.GetHeight() - bounds.GetHeight());
							break;
						case AnchorPresets::VerticalStretchLeft:
							bounds.Location = Float2::Zero;
							bounds.Size = Float2(bounds.GetLeft(), parentBounds.GetHeight());
							break;
						case AnchorPresets::VerticalStretchCenter:
							bounds.Location = Float2(parentBounds.GetLeft() * 0.5f - bounds.GetLeft() * 0.5f, 0);
							bounds.Size = Float2(bounds.GetLeft(), parentBounds.GetHeight());
							break;
						case AnchorPresets::VerticalStretchRight:
							bounds.Location = Float2(parentBounds.GetLeft() - bounds.GetLeft(), 0);
							bounds.Size = Float2(bounds.GetLeft(), parentBounds.GetHeight());
							break;
						case AnchorPresets::HorizontalStretchTop:
							bounds.Location = Float2::Zero;
							bounds.Size = Float2(parentBounds.GetLeft(), bounds.GetHeight());
							break;
						case AnchorPresets::HorizontalStretchMiddle:
							bounds.Location = Float2(0, parentBounds.GetHeight() * 0.5f - bounds.GetHeight() * 0.5f);
							bounds.Size = Float2(parentBounds.GetLeft(), bounds.GetHeight());
							break;
						case AnchorPresets::HorizontalStretchBottom:
							bounds.Location = Float2(0, parentBounds.GetHeight() - bounds.GetHeight());
							bounds.Size = Float2(parentBounds.GetLeft(), bounds.GetHeight());
							break;
						case AnchorPresets::StretchAll:
							bounds.Location = Float2::Zero;
							bounds.Size = parentBounds.Size;
							break;
						default: ENGINE_UNREACHABLE_CODE();
						}
						bounds.Location += parentBounds.Location;
					}
					Bounds = bounds;
				}
				if (setPivotToo)
				{
					Pivot = (anchorMin + anchorMax) / Float2(2);
				}
				if (m_Parent != nullptr)
				{
					m_Parent->PerformLayout();
				}
				return;
			}
		}
	}

}
