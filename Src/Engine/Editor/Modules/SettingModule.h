#pragma once

#include "EditorModule.h"
#include "Runtime/Core/Math/Vector2.h"
#include "Runtime/Core/Types/Pair.h"
#include "Runtime/Core/Types/Strings/String.h"

namespace SE
{
	class Texture;
	struct SpriteHandle;
	class SpriteAtlas;
	class Style;
}

namespace SE::Editor
{

	class SettingModule : public EditorModule
	{
	public:

		struct IconPack
		{
			String name;
			SpriteHandle* value = nullptr;
			Texture* texture = nullptr;
			Int2 position = Int2(0, 0);

			IconPack() : name()
			{

			}

			IconPack(String name, SpriteHandle* value) : name(name), value(value)
			{
			}
		};

		explicit SettingModule(EditorApp* editor);

		int InitOrder() override;

		void OnInit() override;

		void OnUpdate() override;

	private:
	    void SetupStyle();

		Style* CreateDefaultStyle();

        /// <summary>
        /// Creates the light style (2nd default).
        /// </summary>
        /// <returns>The style object.</returns>
		Style* CreateLightStyle();

		void ImportAsset();
		void LoadIcon();
		void UploadIcon();


		SpriteAtlas* m_IconAtlas = nullptr;
		bool m_IconAtlasUpload = false;
		List<IconPack*> m_LoadedIcons;

	};

} // SE
