#pragma once
#include "Core/Types/Strings/String.h"
#include "Runtime/Render/2D/FontReference.h"

namespace SE
{
	class FontAsset;
}

namespace SE::Editor
{

	class EditorAssets
	{
		friend class SettingModule;
	private:
		static FontAsset* GetDefaultFont();
		static FontAsset* GetConsoleFont();

		static FontReference m_titleFont;
		static FontReference m_largeFont;
		static FontReference m_mediumFont;
		static FontReference m_smallFont;
		static FontReference m_outputLogFont;

	public:
		struct SegMDL2Icons
		{
			static String Cancel;
			static String ChromeMinimize;
			static String ChromeMaximize;
			static String ChromeRestore;
			static String ChromeClose;
		};


		static String AssetsPath;

		/// <summary>
		/// The icons atlas.
		/// </summary>
		static String IconsPath;

		/// <summary>
		/// The window icons font.
		/// </summary>
		static String WindowIconsFont;

		/// <summary>
		/// The primary font.
		/// </summary>
		static String PrimaryFont;

		/// <summary>
		/// The Inconsolata Regular font.
		/// </summary>
		static String InconsolataRegularFont;

		/// <summary>
		/// The window icon.
		/// </summary>
		static String WindowIcon;

        /// <summary>
        /// Gets or sets the title font for editor UI.
        /// </summary>
        static FontReference GetTitleFont();
		static void SetTitleFont(const FontReference& value);

        /// <summary>
        /// Gets or sets the large font for editor UI.
        /// </summary>
		static FontReference GetLargeFont();
        static void SetLargeFont(const FontReference& value);

        /// <summary>
        /// Gets or sets the medium font for editor UI.
        /// </summary>
		static FontReference GetMediumFont();
		static void SetMediumFont(const FontReference& value);

        /// <summary>
        /// Gets or sets the small font for editor UI.
        /// </summary>
		static FontReference GetSmallFont();
		static void SetSmallFont(const FontReference& value);

	};

} // SE
