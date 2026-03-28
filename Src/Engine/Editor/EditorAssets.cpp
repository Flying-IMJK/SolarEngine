
#include "EditorAssets.h"
#include "EditorIcons.h"
#include "Runtime/Render/2D/FontAsset.h"

namespace SE::Editor
{
	String EditorAssets::SegMDL2Icons::Cancel = SE_TEXT("\uE711");
	String EditorAssets::SegMDL2Icons::ChromeMinimize = SE_TEXT("\uE921");
	String EditorAssets::SegMDL2Icons::ChromeMaximize = SE_TEXT("\uE922");
	String EditorAssets::SegMDL2Icons::ChromeRestore = SE_TEXT("\uE923");
	String EditorAssets::SegMDL2Icons::ChromeClose = SE_TEXT("\uE8BB");


	String EditorAssets::AssetsPath = SE_TEXT("Assets");

	String EditorAssets::IconsPath = SE_TEXT("Assets/Icon");
	String EditorAssets::WindowIconsFont = SE_TEXT("Assets/Fonts/SegMDL2");

	String EditorAssets::PrimaryFont = SE_TEXT("Assets/Fonts/Roboto-Regular");
	String EditorAssets::InconsolataRegularFont = SE_TEXT("Assets/Fonts/Roboto-Regular");

	String EditorAssets::WindowIcon = SE_TEXT("Assets/Icon/Engine_Icon");

	FontReference EditorAssets::m_titleFont;
	FontReference EditorAssets::m_largeFont;
	FontReference EditorAssets::m_mediumFont;
	FontReference EditorAssets::m_smallFont;
	FontReference EditorAssets::m_outputLogFont;

	FontAsset* EditorAssets::GetDefaultFont()
	{
		return AssetContent::LoadAsyncInternal<FontAsset>(PrimaryFont.Get());
	}

	FontAsset* EditorAssets::GetConsoleFont()
	{
		return AssetContent::LoadAsyncInternal<FontAsset>(InconsolataRegularFont.Get());
	}

	FontReference EditorAssets::GetTitleFont()
	{
		return m_titleFont;
	}

	void EditorAssets::SetTitleFont(const FontReference& value)
	{
		if (!value.Font)
			m_titleFont.Font = GetDefaultFont();
		else
			m_titleFont = value;
	}

	FontReference EditorAssets::GetLargeFont()
	{
		return m_largeFont;
	}

	void EditorAssets::SetLargeFont(const FontReference& value)
	{
		if (!value.Font)
			m_largeFont.Font = GetDefaultFont();
		else
			m_largeFont = value;
	}

	FontReference EditorAssets::GetMediumFont()
	{
		return m_mediumFont;
	}

	void EditorAssets::SetMediumFont(const FontReference& value)
	{
		if (!value.Font)
			m_mediumFont.Font = GetDefaultFont();
		else
			m_mediumFont = value;
	}

	FontReference EditorAssets::GetSmallFont()
	{
		return m_smallFont;
	}

	void EditorAssets::SetSmallFont(const FontReference& value)
	{
		if (!value.Font)
			m_smallFont.Font = GetDefaultFont();
		else
			m_smallFont = value;
	}

	// 12px
    SpriteHandle EditorIcons::DragBar12;
    SpriteHandle EditorIcons::Search12;
    SpriteHandle EditorIcons::WindowDrag12;
    SpriteHandle EditorIcons::CheckBoxIntermediate12;
    SpriteHandle EditorIcons::ArrowRight12;
    SpriteHandle EditorIcons::Settings12;
    SpriteHandle EditorIcons::Cross16;
    SpriteHandle EditorIcons::CheckBoxTick12;
    SpriteHandle EditorIcons::ArrowDown12;

    // 32px
    SpriteHandle EditorIcons::Scalar32;
    SpriteHandle EditorIcons::Translate32;
    SpriteHandle EditorIcons::Rotate32;
    SpriteHandle EditorIcons::Scale32;
    SpriteHandle EditorIcons::Grid32;
    SpriteHandle EditorIcons::Flax32;
    SpriteHandle EditorIcons::RotateSnap32;
    SpriteHandle EditorIcons::ScaleSnap32;
    SpriteHandle EditorIcons::Globe32;
    SpriteHandle EditorIcons::CamSpeed32;
    SpriteHandle EditorIcons::Link32;
    SpriteHandle EditorIcons::Add32;
    SpriteHandle EditorIcons::Left32;
    SpriteHandle EditorIcons::Right32;
    SpriteHandle EditorIcons::Up32;
    SpriteHandle EditorIcons::Down32;
    SpriteHandle EditorIcons::FolderClosed32;
    SpriteHandle EditorIcons::FolderOpen32;
    SpriteHandle EditorIcons::Folder32;
    SpriteHandle EditorIcons::CameraFill32;
    SpriteHandle EditorIcons::Search32;
    SpriteHandle EditorIcons::Info32;
    SpriteHandle EditorIcons::Warning32;
    SpriteHandle EditorIcons::Error32;
    SpriteHandle EditorIcons::Bone32;
    SpriteHandle EditorIcons::BoneFull32;
	SpriteHandle EditorIcons::LogWindow32;

    // Visject
    SpriteHandle EditorIcons::VisjectBoxOpen32;
    SpriteHandle EditorIcons::VisjectBoxClosed32;
    SpriteHandle EditorIcons::VisjectArrowOpen32;
    SpriteHandle EditorIcons::VisjectArrowClosed32;

    // 64px
    SpriteHandle EditorIcons::Flax64;
    SpriteHandle EditorIcons::Save64;
    SpriteHandle EditorIcons::Play64;
    SpriteHandle EditorIcons::Stop64;
    SpriteHandle EditorIcons::Pause64;
    SpriteHandle EditorIcons::Skip64;
    SpriteHandle EditorIcons::Info64;
    SpriteHandle EditorIcons::Error64;
    SpriteHandle EditorIcons::Warning64;
    SpriteHandle EditorIcons::AddFile64;
    SpriteHandle EditorIcons::DeleteFile64;
    SpriteHandle EditorIcons::Import64;
    SpriteHandle EditorIcons::Left64;
    SpriteHandle EditorIcons::Right64;
    SpriteHandle EditorIcons::Up64;
    SpriteHandle EditorIcons::Down64;
    SpriteHandle EditorIcons::Undo64;
    SpriteHandle EditorIcons::Redo64;
    SpriteHandle EditorIcons::Translate64;
    SpriteHandle EditorIcons::Rotate64;
    SpriteHandle EditorIcons::Scale64;
    SpriteHandle EditorIcons::Refresh64;
    SpriteHandle EditorIcons::Shift64;
    SpriteHandle EditorIcons::Code64;
    SpriteHandle EditorIcons::Folder64;
    SpriteHandle EditorIcons::CenterView64;
    SpriteHandle EditorIcons::Image64;
    SpriteHandle EditorIcons::Camera64;
    SpriteHandle EditorIcons::Docs64;
    SpriteHandle EditorIcons::Search64;
    SpriteHandle EditorIcons::Bone64;
    SpriteHandle EditorIcons::Link64;
    SpriteHandle EditorIcons::Build64;
    SpriteHandle EditorIcons::Add64;
    SpriteHandle EditorIcons::ShipIt64;
    SpriteHandle EditorIcons::SplineFree64;
    SpriteHandle EditorIcons::SplineLinear64;
    SpriteHandle EditorIcons::SplineAligned64;
    SpriteHandle EditorIcons::SplineSmoothIn64;

    // 96px
    SpriteHandle EditorIcons::Toolbox96;
    SpriteHandle EditorIcons::Paint96;
    SpriteHandle EditorIcons::Foliage96;
    SpriteHandle EditorIcons::Terrain96;

    // 128px
    SpriteHandle EditorIcons::AndroidSettings128;
    SpriteHandle EditorIcons::PlaystationSettings128;
    SpriteHandle EditorIcons::InputSettings128;
    SpriteHandle EditorIcons::PhysicsSettings128;
    SpriteHandle EditorIcons::CSharpScript128;
    SpriteHandle EditorIcons::Folder128;
    SpriteHandle EditorIcons::WindowsIcon128;
    SpriteHandle EditorIcons::LinuxIcon128;
    SpriteHandle EditorIcons::UWPSettings128;
    SpriteHandle EditorIcons::XBOXSettings128;
    SpriteHandle EditorIcons::LayersTagsSettings128;
    SpriteHandle EditorIcons::GraphicsSettings128;
    SpriteHandle EditorIcons::CPPScript128;
    SpriteHandle EditorIcons::Plugin128;
    SpriteHandle EditorIcons::XBoxScarletIcon128;
    SpriteHandle EditorIcons::AssetShadow128;
    SpriteHandle EditorIcons::WindowsSettings128;
    SpriteHandle EditorIcons::TimeSettings128;
    SpriteHandle EditorIcons::GameSettings128;
    SpriteHandle EditorIcons::VisualScript128;
    SpriteHandle EditorIcons::Document128;
    SpriteHandle EditorIcons::XBoxOne128;
    SpriteHandle EditorIcons::UWPStore128;
    SpriteHandle EditorIcons::ColorWheel128;
    SpriteHandle EditorIcons::LinuxSettings128;
    SpriteHandle EditorIcons::NavigationSettings128;
    SpriteHandle EditorIcons::AudioSettings128;
    SpriteHandle EditorIcons::BuildSettings128;
    SpriteHandle EditorIcons::Scene128;
    SpriteHandle EditorIcons::AndroidIcon128;
    SpriteHandle EditorIcons::PS4Icon128;
    SpriteHandle EditorIcons::PS5Icon128;
    SpriteHandle EditorIcons::MacOSIcon128;
    SpriteHandle EditorIcons::IOSIcon128;
    SpriteHandle EditorIcons::FlaxLogo128;
    SpriteHandle EditorIcons::SwitchIcon128;
    SpriteHandle EditorIcons::SwitchSettings128;
    SpriteHandle EditorIcons::LocalizationSettings128;
    SpriteHandle EditorIcons::Json128;
    SpriteHandle EditorIcons::AppleSettings128;

} // SE