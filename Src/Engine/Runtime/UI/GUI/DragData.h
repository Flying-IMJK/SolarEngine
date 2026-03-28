#pragma once

namespace SE
{
	enum class DragDataType
	{
		Text,
		File
	};

	class SE_API_RUNTIME DragData : public IType
	{
		SE_CLASS(DragData, IType)
	public:
		~DragData() override = default;
		DragData() = default;
		virtual DragDataType GetDataType() const = 0;
	};

	class SE_API_RUNTIME DragDataText : public DragData
	{
		SE_CLASS(DragDataText, DragData)
	public:
		/// <summary>
		/// The text.
		/// </summary>
		String Text = String::Empty;

		DragDataText() = default;

		/// <summary>
		/// Initializes a new instance of the <see cref="DragDataText"/> class.
		/// </summary>
		/// <param name="text">The text.</param>
		DragDataText(String text) : Text(text)
		{

		}

		DragDataType GetDataType() const override { return DragDataType::Text; }
	};

	/// <summary>
	/// The drag and drop files.
	/// </summary>
	/// <seealso cref="FlaxEngine.GUI.DragData" />
	class SE_API_RUNTIME DragDataFiles : public DragData
	{
		SE_CLASS(DragDataFiles, DragData)
	public:
		/// <summary>
		/// The file paths collection.
		/// </summary>
		List<String> Files;

		DragDataType GetDataType() const override { return DragDataType::File; }

		DragDataFiles() = default;
	};

}
