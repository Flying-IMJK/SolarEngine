
#pragma once

//#include "Engine/Core/Config/Settings.h"
#include "TextureGroup.h"

namespace SE
{
	/*/// <summary>
	/// Content streaming settings.
	/// </summary>
	class SE_API_RUNTIME StreamingSettings : public SettingsBase
	{
		SE_CLASS(StreamingSettings, SettingsBase);
	public:

		/// <summary>
		/// Textures streaming configuration (per-group).
		/// </summary>
		API_FIELD(Attributes="EditorOrder(100), EditorDisplay(\"Textures\")")
		List<TextureGroup, InlinedAllocation<32>> TextureGroups;

	public:

		/// <summary>
		/// Gets the instance of the settings asset (default value if missing). Object returned by this method is always loaded with valid data to use.
		/// </summary>
		static StreamingSettings* Get();

		// [SettingsBase]
		void Apply() override;
		void Deserialize(DeserializeStream& stream, ISerializeModifier* modifier) final override;
	};*/
}