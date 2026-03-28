
#include "AssetOperate.h"

#include "Runtime/UI/GUI/Common/Label.h"

namespace SE::Editor
{
	bool AssetOperate::GetIsVirtual()
	{
		return false;
	}

	bool AssetOperate::IsVirtualOperate()
	{
		return GetIsVirtual() && CanExport == false;
	}

	bool AssetOperate::AcceptsAsset(TypeID typeID, StringView path)
	{
		return GetAssetType() == typeID && path.EndsWith(FileExtension.operator->());
	}

	void AssetOperate::OnThumbnailDrawBegin(ThumbnailRequest* request, ContainerControl* guiRoot, GPUContext* context)
	{
		Label* label = New<Label>();
		label->Text = Name;
		label->AnchorPreset = AnchorPresets::StretchAll;
		label->Offsets = Margin::Zero;
		label->Wrapping = TextWrapping::WrapWords;

		guiRoot->AddChild(label);

	}
} // SE