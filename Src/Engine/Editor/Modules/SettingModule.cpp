//
// Created by 10303 on 25-8-13.
//

#include "SettingModule.h"

#include "Core/Platform/Windows/WindowsFileSystem.h"
#include "Editor/EditorIcons.h"
#include "Editor/EditorAssets.h"

#include "Core/Types/Strings/String.h"

#include "Runtime/UI/GUI/Style.h"
#include "Runtime/EngineContext.h"
#include "Runtime/Graphics/GPUContext.h"
#include "Runtime/Graphics/GPUDevice.h"
#include "Runtime/Render/2D/SpriteAtlasPacker.h"
#include "Runtime/Render/Assets/Texture/Texture.h"
#include "Runtime/Resource/Factories/IAssetFactory.h"
#include "Runtime/Resource/Importers/AssetsImportingSystem.h"

namespace SE::Editor
{
    void LoadIcon();
	List<SettingModule::IconPack>& GetIconDefine();
	void ImportAsset();

	SettingModule::SettingModule(EditorApp* editor) : EditorModule(editor)
	{

	}

	int SettingModule::InitOrder()
	{
		return -100;
	}

    void SettingModule::OnInit()
    {
    	ImportAsset();
    	LoadIcon();
        SetupStyle();
    }

    void SettingModule::OnUpdate()
    {
	    if (!m_IconAtlasUpload)
	    {
		    UploadIcon();
	    }
    }

    void SettingModule::SetupStyle()
    {
        // If a non-default style was chosen, switch to that style
        /*String styleName = themeOptions.SelectedStyle;
        if (styleName != ThemeOptions.DefaultName && styleName != ThemeOptions.LightDefault && themeOptions.Styles.TryGetValue(styleName, out var style) && style != null)
        {
            // Setup defaults for newly added components that might be missing
            if (style.Selection == Color.Transparent && style.SelectionBorder == Color.Transparent)
            {
                // [Deprecated on 6.03.2024, expires on 6.03.2026]
                style.Selection = Color.Orange * 0.4f;
                style.SelectionBorder = Color.Orange;
            }

            Style.Current = style;
        }
        else*/
        {
            /*if (styleName == ThemeOptions.LightDefault)
            {
                Style::Current = CreateLightStyle();
            }
            else
            {
                Style::Current = CreateDefaultStyle();
            }*/

        	Style::Current = CreateLightStyle();
        }

        // Ensure custom fonts are valid, reset if not
        if (Style::Current->FontTitle == nullptr)
            Style::Current->FontTitle = EditorAssets::GetTitleFont().GetFont();
        if (Style::Current->FontSmall == nullptr)
            Style::Current->FontSmall = EditorAssets::GetSmallFont().GetFont();
        if (Style::Current->FontMedium == nullptr)
            Style::Current->FontMedium = EditorAssets::GetMediumFont().GetFont();
        if (Style::Current->FontLarge == nullptr)
            Style::Current->FontLarge = EditorAssets::GetLargeFont().GetFont();

        // Set fallback fonts
        /*var fallbackFonts = Options.Interface.FallbackFonts;
        if (fallbackFonts == null || fallbackFonts.Length == 0 || fallbackFonts.All(x => x == null))
            fallbackFonts = GameSettings.Load<GraphicsSettings>().FallbackFonts;
        Style::FallbackFonts = fallbackFonts;*/
    }

    Style* SettingModule::CreateDefaultStyle()
    {
        Style* style = New<Style>();

        style->Background = Color::FromRGBA(0x1CFF1C1C);
        style->LightBackground = Color::FromRGBA(0x2DFF2D30);
        style->Foreground = Color::FromRGBA(0xFFFFFFFF);
        style->ForegroundGrey = Color::FromRGBA(0xA9FFA9B3);
        style->ForegroundDisabled = Color::FromRGBA(0x78FF7883);
        style->ForegroundViewport = Color::FromRGBA(0xFFFFFFFF);
        style->BackgroundHighlighted = Color::FromRGBA(0x54FF545C);
        style->BorderHighlighted = Color::FromRGBA(0x6AFF6A75);
        style->BackgroundSelected = Color::FromRGBA(0x7AFF00CC);
        style->BorderSelected = Color::FromRGBA(0x97FF1CEA);
        style->BackgroundNormal = Color::FromRGBA(0x3FFF3F46);
        style->BorderNormal = Color::FromRGBA(0x54FF545C);
        style->TextBoxBackground = Color::FromRGBA(0x33FF3337);
        style->TextBoxBackgroundSelected = Color::FromRGBA(0x3FFF3F46);
        style->CollectionBackgroundColor = Color::FromRGBA(0xCC14CCCC);
        style->ProgressNormal = Color::FromRGBA(0xd3FF0a28);
        style->Selection = Colors::Orange * 0.4f;
        style->SelectionBorder = Colors::Orange;

        style->Statusbar.PlayMode = Color::FromRGBA(0x91FF2F35);
        style->Statusbar.Failed = Color::FromRGBA(0x24FF9C24);
        style->Statusbar.Loading = Color::FromRGBA(0x2DFF2D30);

        // Fonts
        style->FontTitle = EditorAssets::GetTitleFont().GetFont();
        style->FontLarge = EditorAssets::GetLargeFont().GetFont();
        style->FontMedium = EditorAssets::GetMediumFont().GetFont();
        style->FontSmall = EditorAssets::GetSmallFont().GetFont();

        // Icons
        style->ArrowDown = EditorIcons::ArrowDown12;
        style->ArrowRight = EditorIcons::ArrowRight12;
        style->Search = EditorIcons::Search12;
        style->Settings = EditorIcons::Settings12;
        style->Cross = &EditorIcons::Cross16;
        style->CheckBoxIntermediate = EditorIcons::CheckBoxIntermediate12;
        style->CheckBoxTick = EditorIcons::CheckBoxTick12;
        style->StatusBarSizeGrip = EditorIcons::WindowDrag12;
        style->Translate = EditorIcons::Translate32;
        style->Rotate = EditorIcons::Rotate32;
        style->Scale = EditorIcons::Scale32;
        style->Scalar = EditorIcons::Scalar32;

        // style->SharedTooltip = New<Tooltip>();

        style->DragWindow = style->BackgroundSelected * 0.7f;
        return style;
    }

    Style* SettingModule::CreateLightStyle()
    {
        Style* style = New<Style>();

        style->Background = Color(0.92f, 0.92f, 0.92f, 1.0f);
        style->LightBackground = Color(0.84f, 0.84f, 0.88f, 1.0f);
        style->DragWindow = Color(0.0f, 0.26f, 0.43f, 0.70f);
        style->Foreground = Color(1.0f, 1.0f, 1.0f, 1.0f);
        style->ForegroundGrey = Color(0.30f, 0.30f, 0.31f, 1.0f);
        style->ForegroundDisabled = Color(0.45f, 0.45f, 0.49f, 1.0f);
        style->ForegroundViewport = Color(1.0f, 1.0f, 1.0f, 1.0f);
        style->BackgroundHighlighted = Color(0.59f, 0.59f, 0.64f, 1.0f);
        style->BorderHighlighted = Color(0.50f, 0.50f, 0.55f, 1.0f);
        style->BackgroundSelected = Color(0.00f, 0.46f, 0.78f, 0.78f);
        style->BorderSelected = Color(0.11f, 0.57f, 0.88f, 0.65f);
        style->BackgroundNormal = Color(0.67f, 0.67f, 0.75f, 1.0f);
        style->BorderNormal = Color(0.59f, 0.59f, 0.64f, 1.0f);
		style->TextColor = Color(0.0f, 0.0f, 0.0f, 1.0f);
        style->TextBoxBackground = Color(0.75f, 0.75f, 0.81f, 1.0f);
        style->TextBoxBackgroundSelected = Color(0.73f, 0.73f, 0.80f, 1.0f);
        style->CollectionBackgroundColor = Color(0.85f, 0.85f, 0.88f, 1.0f);
        style->ProgressNormal = Color(0.03f, 0.65f, 0.12f, 1.0f);
        style->Selection = Colors::Orange * 0.4f;
        style->SelectionBorder = Colors::Orange;

        // Fonts
        style->FontTitle = EditorAssets::GetTitleFont().GetFont();
        style->FontLarge = EditorAssets::GetLargeFont().GetFont();
        style->FontMedium = EditorAssets::GetMediumFont().GetFont();
        style->FontSmall = EditorAssets::GetSmallFont().GetFont();

        // Icons
        style->ArrowDown = EditorIcons::ArrowDown12;
        style->ArrowRight = EditorIcons::ArrowRight12;
        style->Search = EditorIcons::Search12;
        style->Settings = EditorIcons::Settings12;
        style->Cross = &EditorIcons::Cross16;
        style->CheckBoxIntermediate = EditorIcons::CheckBoxIntermediate12;
        style->CheckBoxTick = EditorIcons::CheckBoxTick12;
        style->StatusBarSizeGrip = EditorIcons::WindowDrag12;
        style->Translate = EditorIcons::Translate32;
        style->Rotate = EditorIcons::Rotate32;
        style->Scale = EditorIcons::Scale32;
        style->Scalar = EditorIcons::Scalar32;

        // SharedTooltip = New<Tooltip>();
        return style;
    }

	void SettingModule::ImportAsset()
    {
    	String assetsPath = String::Format(SE_TEXT("{0}/{1}"), EngineContext::ProjectCacheFolder, EditorAssets::AssetsPath);

    	List<String> file;
    	FileSystem::DirectoryGetFiles(file, assetsPath, nullptr);
		for (String fileName : file)
		{
			String fileExtension = FileSystem::GetExtension(fileName);
			if (fileExtension == ASSET_FILES_EXTENSION)
			{
				continue;
			}

			auto importer = AssetsImporting::GetImporter(fileExtension);
			if (importer == nullptr)
			{
				continue;
			}

			String outPath = fileName;
			FileSystem::ReplaceExtension(outPath, ASSET_FILES_EXTENSION);
			AssetsImporting::ImportIfEdited(fileName, outPath);
		}

    	EditorAssets::m_titleFont = FontReference(EditorAssets::GetDefaultFont(), 18);
    	EditorAssets::m_largeFont = FontReference(EditorAssets::GetDefaultFont(), 14);
    	EditorAssets::m_mediumFont = FontReference(EditorAssets::GetDefaultFont(), 9);
    	EditorAssets::m_smallFont = FontReference(EditorAssets::GetDefaultFont(), 9);
    	EditorAssets::m_outputLogFont = FontReference(EditorAssets::GetConsoleFont(), 10);
	}

    void SettingModule::LoadIcon()
    {
	    List<IconPack*> icons;
		StringBuilder path;
    	// Find icons
    	for (IconPack &iconDefine : GetIconDefine())
    	{
    		path.Clear();
    		path.Append(EditorAssets::IconsPath);
    		path.Append("/");
    		path.Append(iconDefine.name);
    		auto icon = AssetContent::LoadAsyncInternal<Texture>(path.ToString());

    		if (icon == nullptr)
    		{
    			LOG_WARNING("Setting", "not find icon \'{0}\'.", iconDefine.name);
    			continue;
    		}

    		iconDefine.texture = icon;

    		icons.Add(&iconDefine);
    	}

		m_LoadedIcons.Clear();
    	List<IconPack*> loadFailedIcons;
    	while (icons.Count() > 0)
    	{
    		for (int i = 0; i < icons.Count(); i++)
    		{
    			IconPack* iconPack = icons[i];

    			if (!iconPack->texture->IsLoaded())
    			{
    				continue;
    			}

    			if (iconPack->texture->LastLoadFailed())
    			{
    				LOG_WARNING("Setting", "Failed to load icon \'{0}\'.", iconPack->name);
    				loadFailedIcons.Add(iconPack);
    			}
    			else
    			{
    				m_LoadedIcons.Add(iconPack);
    			}

    			icons.RemoveAt(i);
    			i--;
    		}
    	}

		m_IconAtlas = AssetContent::CreateVirtualAsset<SpriteAtlas>();

		TextureInitData* initData = TextureInitData::Create(512, 512, 1, PixelFormat::BC3_UNorm);
		m_IconAtlas->Init(initData);

		SpriteAtlasPacker atlasPacker = SpriteAtlasPacker(initData->Width, initData->Height, 2);

		for (int i = 0; i < m_LoadedIcons.Count(); i++)
		{
			IconPack* iconPack = m_LoadedIcons[i];

			atlasPacker.InsertRectangle(iconPack->texture->Width(), iconPack->texture->Height(), i);
		}

		atlasPacker.PackRectangles();

		Float2 atlasSize = m_IconAtlas->Size();
		for (int i = 0; i < m_LoadedIcons.Count(); i++)
		{
			IconPack* iconPack = m_LoadedIcons[i];

			SpriteAtlasPacker::Rect rectangle = atlasPacker.GetRectangle(i);

			Sprite sprite;
			sprite.Area = Rectangle(rectangle.x / atlasSize.x, rectangle.y / atlasSize.y, rectangle.width / atlasSize.x, rectangle.height / atlasSize.y);
			sprite.Name = iconPack->name;
			SpriteHandle handle = m_IconAtlas->AddSprite(sprite);

			*iconPack->value = handle;
			iconPack->position = Int2(rectangle.x, rectangle.y);
		}
    }

    void SettingModule::UploadIcon()
    {
		for (int i = 0; i < m_LoadedIcons.Count(); i++)
		{
			IconPack* iconPack = m_LoadedIcons[i];

			if (iconPack->texture->GetResidentMipLevels() < iconPack->texture->GetMipLevels())
			{
				return;
			}
		}

		if (m_IconAtlas->GetResidentMipLevels() <= 0)
		{
			return;
		}

		m_IconAtlasUpload = true;

		GPUContext* gpuContext = GPUDevice::instance->GetMainContext();
		for (int i = 0; i < m_LoadedIcons.Count(); i++)
		{
			IconPack* iconPack = m_LoadedIcons[i];
			gpuContext->CopyTexture(m_IconAtlas->GetTexture(), 0, iconPack->position.x, iconPack->position.y, 0,
				iconPack->texture->GetTexture(), 0);
		}
    }

#define DEFINE_ICON_PACK(icon) SettingModule::IconPack(StringView(SE_TEXT(MACRO_TO_STR(icon))).Substring(13) , &icon)

	List<SettingModule::IconPack>& GetIconDefine()
	{
		static List<SettingModule::IconPack> iconList =
		{
			DEFINE_ICON_PACK(EditorIcons::DragBar12),
			DEFINE_ICON_PACK(EditorIcons::Search12),
			DEFINE_ICON_PACK(EditorIcons::WindowDrag12),
			DEFINE_ICON_PACK(EditorIcons::DragBar12),
			DEFINE_ICON_PACK(EditorIcons::Search12),
			DEFINE_ICON_PACK(EditorIcons::WindowDrag12),
			DEFINE_ICON_PACK(EditorIcons::CheckBoxIntermediate12),
			DEFINE_ICON_PACK(EditorIcons::ArrowRight12),
			DEFINE_ICON_PACK(EditorIcons::Settings12),
			DEFINE_ICON_PACK(EditorIcons::Cross16),
			DEFINE_ICON_PACK(EditorIcons::CheckBoxTick12),
			DEFINE_ICON_PACK(EditorIcons::ArrowDown12),

			DEFINE_ICON_PACK(EditorIcons::Scalar32),
			DEFINE_ICON_PACK(EditorIcons::Translate32),
			DEFINE_ICON_PACK(EditorIcons::Rotate32),
			DEFINE_ICON_PACK(EditorIcons::Scale32),
			DEFINE_ICON_PACK(EditorIcons::Grid32),
			DEFINE_ICON_PACK(EditorIcons::Flax32),
			DEFINE_ICON_PACK(EditorIcons::RotateSnap32),
			DEFINE_ICON_PACK(EditorIcons::ScaleSnap32),
			DEFINE_ICON_PACK(EditorIcons::Globe32),
			DEFINE_ICON_PACK(EditorIcons::CamSpeed32),
			DEFINE_ICON_PACK(EditorIcons::Link32),
			DEFINE_ICON_PACK(EditorIcons::Add32),
			DEFINE_ICON_PACK(EditorIcons::Left32),
			DEFINE_ICON_PACK(EditorIcons::Right32),
			DEFINE_ICON_PACK(EditorIcons::Up32),
			DEFINE_ICON_PACK(EditorIcons::Down32),
			DEFINE_ICON_PACK(EditorIcons::FolderClosed32),
			DEFINE_ICON_PACK(EditorIcons::FolderOpen32),
			DEFINE_ICON_PACK(EditorIcons::Folder32),
			DEFINE_ICON_PACK(EditorIcons::CameraFill32),
			DEFINE_ICON_PACK(EditorIcons::Search32),
			DEFINE_ICON_PACK(EditorIcons::Info32),
			DEFINE_ICON_PACK(EditorIcons::Warning32),
			DEFINE_ICON_PACK(EditorIcons::Error32),
			DEFINE_ICON_PACK(EditorIcons::Bone32),
			DEFINE_ICON_PACK(EditorIcons::BoneFull32),
			DEFINE_ICON_PACK(EditorIcons::LogWindow32),

			DEFINE_ICON_PACK(EditorIcons::VisjectBoxOpen32),
			DEFINE_ICON_PACK(EditorIcons::VisjectBoxClosed32),
			DEFINE_ICON_PACK(EditorIcons::VisjectArrowOpen32),
			DEFINE_ICON_PACK(EditorIcons::VisjectArrowClosed32),

			DEFINE_ICON_PACK(EditorIcons::Flax64),
			DEFINE_ICON_PACK(EditorIcons::Save64),
			DEFINE_ICON_PACK(EditorIcons::Play64),
			DEFINE_ICON_PACK(EditorIcons::Stop64),
			DEFINE_ICON_PACK(EditorIcons::Pause64),
			DEFINE_ICON_PACK(EditorIcons::Skip64),
			DEFINE_ICON_PACK(EditorIcons::Info64),
			DEFINE_ICON_PACK(EditorIcons::Error64),
			DEFINE_ICON_PACK(EditorIcons::Warning64),
			DEFINE_ICON_PACK(EditorIcons::AddFile64),
			DEFINE_ICON_PACK(EditorIcons::DeleteFile64),
			DEFINE_ICON_PACK(EditorIcons::Import64),
			DEFINE_ICON_PACK(EditorIcons::Left64),
			DEFINE_ICON_PACK(EditorIcons::Right64),
			DEFINE_ICON_PACK(EditorIcons::Up64),
			DEFINE_ICON_PACK(EditorIcons::Down64),
			DEFINE_ICON_PACK(EditorIcons::Undo64),
			DEFINE_ICON_PACK(EditorIcons::Redo64),
			DEFINE_ICON_PACK(EditorIcons::Translate64),
			DEFINE_ICON_PACK(EditorIcons::Rotate64),
			DEFINE_ICON_PACK(EditorIcons::Scale64),
			DEFINE_ICON_PACK(EditorIcons::Refresh64),
			DEFINE_ICON_PACK(EditorIcons::Shift64),
			DEFINE_ICON_PACK(EditorIcons::Code64),
			DEFINE_ICON_PACK(EditorIcons::Folder64),
			DEFINE_ICON_PACK(EditorIcons::CenterView64),
			DEFINE_ICON_PACK(EditorIcons::Image64),
			DEFINE_ICON_PACK(EditorIcons::Camera64),
			DEFINE_ICON_PACK(EditorIcons::Docs64),
			DEFINE_ICON_PACK(EditorIcons::Search64),
			DEFINE_ICON_PACK(EditorIcons::Bone64),
			DEFINE_ICON_PACK(EditorIcons::Link64),
			DEFINE_ICON_PACK(EditorIcons::Build64),
			DEFINE_ICON_PACK(EditorIcons::Add64),
			DEFINE_ICON_PACK(EditorIcons::ShipIt64),
			DEFINE_ICON_PACK(EditorIcons::SplineFree64),
			DEFINE_ICON_PACK(EditorIcons::SplineLinear64),
			DEFINE_ICON_PACK(EditorIcons::SplineAligned64),
			DEFINE_ICON_PACK(EditorIcons::SplineSmoothIn64),

			DEFINE_ICON_PACK(EditorIcons::Toolbox96),
			DEFINE_ICON_PACK(EditorIcons::Paint96),
			DEFINE_ICON_PACK(EditorIcons::Foliage96),
			DEFINE_ICON_PACK(EditorIcons::Terrain96),

			DEFINE_ICON_PACK(EditorIcons::AndroidSettings128),
			DEFINE_ICON_PACK(EditorIcons::PlaystationSettings128),
			DEFINE_ICON_PACK(EditorIcons::InputSettings128),
			DEFINE_ICON_PACK(EditorIcons::PhysicsSettings128),
			DEFINE_ICON_PACK(EditorIcons::CSharpScript128),
			DEFINE_ICON_PACK(EditorIcons::Folder128),
			DEFINE_ICON_PACK(EditorIcons::WindowsIcon128),
			DEFINE_ICON_PACK(EditorIcons::LinuxIcon128),
			DEFINE_ICON_PACK(EditorIcons::UWPSettings128),
			DEFINE_ICON_PACK(EditorIcons::XBOXSettings128),
			DEFINE_ICON_PACK(EditorIcons::LayersTagsSettings128),
			DEFINE_ICON_PACK(EditorIcons::GraphicsSettings128),
			DEFINE_ICON_PACK(EditorIcons::CPPScript128),
			DEFINE_ICON_PACK(EditorIcons::Plugin128),
			DEFINE_ICON_PACK(EditorIcons::XBoxScarletIcon128),
			DEFINE_ICON_PACK(EditorIcons::AssetShadow128),
			DEFINE_ICON_PACK(EditorIcons::WindowsSettings128),
			DEFINE_ICON_PACK(EditorIcons::TimeSettings128),
			DEFINE_ICON_PACK(EditorIcons::GameSettings128),
			DEFINE_ICON_PACK(EditorIcons::VisualScript128),
			DEFINE_ICON_PACK(EditorIcons::Document128),
			DEFINE_ICON_PACK(EditorIcons::XBoxOne128),
			DEFINE_ICON_PACK(EditorIcons::UWPStore128),
			DEFINE_ICON_PACK(EditorIcons::ColorWheel128),
			DEFINE_ICON_PACK(EditorIcons::LinuxSettings128),
			DEFINE_ICON_PACK(EditorIcons::NavigationSettings128),
			DEFINE_ICON_PACK(EditorIcons::AudioSettings128),
			DEFINE_ICON_PACK(EditorIcons::BuildSettings128),
			DEFINE_ICON_PACK(EditorIcons::Scene128),
			DEFINE_ICON_PACK(EditorIcons::AndroidIcon128),
			DEFINE_ICON_PACK(EditorIcons::PS4Icon128),
			DEFINE_ICON_PACK(EditorIcons::PS5Icon128),
			DEFINE_ICON_PACK(EditorIcons::MacOSIcon128),
			DEFINE_ICON_PACK(EditorIcons::IOSIcon128),
			DEFINE_ICON_PACK(EditorIcons::FlaxLogo128),
			DEFINE_ICON_PACK(EditorIcons::SwitchIcon128),
			DEFINE_ICON_PACK(EditorIcons::SwitchSettings128),
			DEFINE_ICON_PACK(EditorIcons::LocalizationSettings128),
			DEFINE_ICON_PACK(EditorIcons::Json128),
			DEFINE_ICON_PACK(EditorIcons::AppleSettings128),
		};
		return iconList;
	}

} // SE