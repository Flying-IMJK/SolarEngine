#pragma once

#include "Editor/CustomEditors/LayoutElement.h"
#include "Core/TypeSystem/TypeID.h"

namespace SE
{
	class Control;
	class Label;
	class Button;
	class Spacer;
	class TextBox;
	class CheckBox;
	class FloatValueBox;
	class DoubleValueBox;
	class IntegerValueBox;
	class Slider;
	class ComboBox;
	class EnumComboBox;
	class Image;
	class SpriteHandle;
	class Texture;
	class GPUTexture;
}

namespace SE::Editor
{
	/// <summary>
	/// Button element for user interaction.
	/// </summary>
	class ButtonElement : public LayoutElement
	{
		SE_DEFINE_CLASS_DEFAULT(ButtonElement, LayoutElement)
	public:
		Button* Button = nullptr;
		Control* GetControl() override;
	};

	/// <summary>
	/// Space element for vertical spacing.
	/// </summary>
	class SpaceElement : public LayoutElement
	{
		SE_DEFINE_CLASS_DEFAULT(SpaceElement, LayoutElement)
	public:
		Spacer* Spacer = nullptr;
		void Init(float height);
		Control* GetControl() override;
	};

	/// <summary>
	/// TextBox element for text input.
	/// </summary>
	class TextBoxElement : public LayoutElement
	{
		SE_DEFINE_CLASS_DEFAULT(TextBoxElement, LayoutElement)
	public:
		TextBox* TextBox = nullptr;
		Control* GetControl() override;
	};

	/// <summary>
	/// CheckBox element for boolean values.
	/// </summary>
	class CheckBoxElement : public LayoutElement
	{
		SE_DEFINE_CLASS_DEFAULT(CheckBoxElement, LayoutElement)
	public:
		CheckBox* CheckBox = nullptr;
		Control* GetControl() override;
	};

	/// <summary>
	/// DoubleValue element for double input.
	/// </summary>
	class DoubleValueElement : public LayoutElement
	{
		SE_DEFINE_CLASS_DEFAULT(DoubleValueElement, LayoutElement)
	public:
		DoubleValueBox* ValueBox = nullptr;
		Control* GetControl() override;
	};

	/// <summary>
	/// IntegerValue element for integer input.
	/// </summary>
	class IntegerValueElement : public LayoutElement
	{
		SE_DEFINE_CLASS_DEFAULT(IntegerValueElement, LayoutElement)
	public:
		IntegerValueBox* ValueBox = nullptr;
		Control* GetControl() override;
	};

	/// <summary>
	/// Slider element for range value input.
	/// </summary>
	class SliderElement : public LayoutElement
	{
		SE_DEFINE_CLASS_DEFAULT(SliderElement, LayoutElement)
	public:
		Slider* Slider = nullptr;
		Control* GetControl() override;
	};

	/// <summary>
	/// ComboBox element for dropdown selection.
	/// </summary>
	class ComboBoxElement : public LayoutElement
	{
		SE_DEFINE_CLASS_DEFAULT(ComboBoxElement, LayoutElement)
	public:
		ComboBox* ComboBox = nullptr;
		Control* GetControl() override;
	};

	/// <summary>
	/// Enum element for enum value selection.
	/// </summary>
	class EnumElement : public LayoutElement
	{
		SE_DEFINE_CLASS_DEFAULT(EnumElement, LayoutElement)
	public:
		EnumComboBox* ComboBox = nullptr;
		Control* GetControl() override;
	};

	/// <summary>
	/// Image element for displaying images.
	/// </summary>
	class ImageElement : public LayoutElement
	{
		SE_DEFINE_CLASS_DEFAULT(ImageElement, LayoutElement)
	public:
		Image* Image = nullptr;
		Control* GetControl() override;
	};

	/// <summary>
	/// Custom element template for arbitrary control types.
	/// </summary>
	template<typename T>
	class CustomElement : public LayoutElement
	{
	public:
		T* CustomControl = nullptr;
		Control* GetControl() override { return reinterpret_cast<Control*>(CustomControl); }
	};

} // namespace SE::Editor
