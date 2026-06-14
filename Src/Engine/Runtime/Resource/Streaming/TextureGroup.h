
#pragma once

#include "Core/Types/Strings/String.h"
#include "Core/Serialization/ISerializable.h"
#include "Runtime/Graphics/Textures/GPUSamplerDescription.h"
#if SE_EDITOR
#include "Core/Types/Collections/Dictionary.h"
#endif

namespace SE
{
	/// <summary>
	/// Settings container for a group of textures. Defines the data streaming options and resource quality.
	/// </summary>
	struct TextureGroup : IType
	{
		SE_DEFINE_CLASS_DEFAULT(TextureGroup, IType)
		/// <summary>
		/// The name of the group.
		/// </summary>
//    SE_PROPERTY(API, Attributes="EditorOrder(10)")
		String Name;

		/// <summary>
		/// The default filtering method for samplers using this texture group.
		/// </summary>
//    SE_PROPERTY(API, Attributes="EditorOrder(15)")
		GPUSamplerFilter SamplerFilter = GPUSamplerFilter::Trilinear;

		/// <summary>
		/// The maximum number of samples that can be used to improve the quality of sample footprints that are anisotropic. Higher values improve texturing but reduce performance. Limited by GPU capabilities and used only if SamplerFilter is Anisotropic.
		/// </summary>
//    SE_PROPERTY(API, Attributes="EditorOrder(16), Limit(1, 16), VisibleIf(\"IsAnisotropic\")")
		int32 MaxAnisotropy = 16;

		/// <summary>
		/// The quality scale factor applied to textures in this group. Can be used to increase or decrease textures resolution. In the range 0-1 where 0 means lowest quality, 1 means full quality.
		/// </summary>
//    SE_PROPERTY(API, Attributes="EditorOrder(20), Limit(0, 1)")
		float Quality = 1.0f;

		/// <summary>
		/// The quality scale factor applied when texture is invisible for some time (defined by TimeToInvisible). Used to decrease texture quality when it's not rendered.
		/// </summary>
//    SE_PROPERTY(API, Attributes="EditorOrder(25), Limit(0, 1)")
		float QualityIfInvisible = 0.5f;

		/// <summary>
		/// The time (in seconds) after which texture is considered to be invisible (if it's not rendered by a certain amount of time).
		/// </summary>
//    SE_PROPERTY(API, Attributes="EditorOrder(26), Limit(0)")
		float TimeToInvisible = 20.0f;

		/// <summary>
		/// The minimum amount of loaded mip levels for textures in this group. Defines the amount of the mips that should be always loaded. Higher values decrease streaming usage and keep more mips loaded.
		/// </summary>
//    SE_PROPERTY(API, Attributes="EditorOrder(30), Limit(0, 14)")
		int32 MipLevelsMin = 0;

		/// <summary>
		/// The maximum amount of loaded mip levels for textures in this group. Defines the maximum amount of mips that can be loaded. Overriden per-platform. Lower values reduce texture quality and improve performance.
		/// </summary>
//    SE_PROPERTY(API, Attributes="EditorOrder(40), Limit(1, 14)")
		int32 MipLevelsMax = 14;

		/// <summary>
		/// The loaded mip levels bias for textures in this group. Can be used to increase or decrease the quality of streaming for textures in this group (eg. bump up the quality during cinematic sequence).
		/// </summary>
//    SE_PROPERTY(API, Attributes="EditorOrder(50), Limit(-14, 14)")
		int32 MipLevelsBias = 0;

#if SE_EDITOR
		/// <summary>
		/// The per-platform maximum amount of mip levels for textures in this group. Can be used to strip textures quality when cooking the game for a target platform.
		/// </summary>
//    SE_PROPERTY(API, Attributes="EditorOrder(50)")
		Dictionary<PlatformType, int32> MipLevelsMaxPerPlatform;
#endif
	};
}