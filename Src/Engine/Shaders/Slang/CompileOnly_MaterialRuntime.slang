#include "Shaders/Slang/SolarShaderProgram.slang"

SHADER_PROGRAM(MaterialDeferredStatic,
	SHADER_VS(MaterialStaticVS),
	SHADER_PS(MaterialDeferredPS)
)

SHADER_PROGRAM(MaterialDeferredInstanced,
	SHADER_VS(MaterialInstancedVS),
	SHADER_PS(MaterialDeferredPS)
)

SHADER_PROGRAM(MaterialForwardStatic,
	SHADER_VS(MaterialStaticVS),
	SHADER_PS(MaterialForwardPS)
)

SHADER_PROGRAM(MaterialForwardInstanced,
	SHADER_VS(MaterialInstancedVS),
	SHADER_PS(MaterialForwardPS)
)

// This Program is intentionally not part of the four installed Material
// contracts. It proves that the global Material table may contain resources
// which are inactive in every Program selected by the current generation.
SHADER_PROGRAM(MaterialOptionalResource,
	SHADER_VS(MaterialStaticVS),
	SHADER_PS(MaterialOptionalResourcePS)
)

struct MaterialParameters
{
	bool Enabled;
	int LayerIndex;
	float2 UVScale;
	float3 Emissive;
	float4 Tint;
	float4 BaseColor;
	float Roughness;
	float4x4 Transform;
	float4 ChannelMask;
	Texture2D AlbedoTexture;
	TextureCube EnvironmentTexture;
	Texture2D NormalTexture;
	Texture2D RuntimeTexture;
	Texture2DArray RuntimeTextureArray;
	Texture3D RuntimeTextureVolume;
	TextureCube RuntimeTextureCube;
	Texture2D SceneTexture;
	SamplerState TextureGroupSampler;
	float4 GameplayValue;
	Texture3D GlobalSDFTexture;
	float4 GlobalSDFConstants;
	Texture2D OptionalTexture;
};

struct PerViewParameters
{
	float4 ViewTint;
};

struct PerDrawParameters
{
	float4 DrawOffset;
};

ParameterBlock<MaterialParameters> Material;
ParameterBlock<PerViewParameters> PerView;
ParameterBlock<PerDrawParameters> PerDraw;

struct StaticVertexInput
{
	float3 Position : POSITION;
};

struct InstancedVertexInput
{
	float3 Position : POSITION;
	float4 InstanceOrigin : ATTRIBUTE0;
};

struct VertexOutput
{
	float4 Position : SV_Position;
};

VertexOutput MaterialStaticVS(StaticVertexInput input)
{
	VertexOutput output;
	output.Position = float4(input.Position + PerDraw.DrawOffset.xyz, 1.0);
	return output;
}

VertexOutput MaterialInstancedVS(InstancedVertexInput input)
{
	VertexOutput output;
	output.Position = float4(input.Position + input.InstanceOrigin.xyz + PerDraw.DrawOffset.xyz, 1.0);
	return output;
}

float4 MaterialDeferredPS(VertexOutput input) : SV_Target
{
	float4 resourceValue = Material.AlbedoTexture.SampleLevel(Material.TextureGroupSampler, Material.UVScale, 0.0);
	resourceValue += Material.EnvironmentTexture.SampleLevel(Material.TextureGroupSampler, float3(1.0, 0.0, 0.0), 0.0);
	resourceValue += Material.NormalTexture.Load(int3(0, 0, 0));
	resourceValue += Material.RuntimeTexture.Load(int3(0, 0, 0));
	resourceValue += Material.RuntimeTextureArray.Load(int4(0, 0, 0, 0));
	resourceValue += Material.RuntimeTextureVolume.Load(int4(0, 0, 0, 0));
	resourceValue += Material.RuntimeTextureCube.SampleLevel(Material.TextureGroupSampler, float3(1.0, 0.0, 0.0), 0.0);
	resourceValue += Material.SceneTexture.Load(int3(0, 0, 0));
	resourceValue += Material.GlobalSDFTexture.Load(int4(0, 0, 0, 0));
	float4 value = mul(float4(Material.Emissive, 1.0), Material.Transform);
	value += Material.Enabled ? Material.Tint : Material.ChannelMask;
	value += Material.BaseColor + Material.GameplayValue + Material.GlobalSDFConstants + resourceValue;
	return value * PerView.ViewTint + (Material.Roughness + Material.LayerIndex) * 0.0001;
}

float4 MaterialForwardPS(VertexOutput input) : SV_Target
{
	float4 resourceValue = Material.AlbedoTexture.SampleLevel(Material.TextureGroupSampler, Material.UVScale, 0.0);
	resourceValue += Material.EnvironmentTexture.SampleLevel(Material.TextureGroupSampler, float3(1.0, 0.0, 0.0), 0.0);
	resourceValue += Material.NormalTexture.Load(int3(0, 0, 0));
	resourceValue += Material.RuntimeTexture.Load(int3(0, 0, 0));
	resourceValue += Material.RuntimeTextureArray.Load(int4(0, 0, 0, 0));
	resourceValue += Material.RuntimeTextureVolume.Load(int4(0, 0, 0, 0));
	resourceValue += Material.RuntimeTextureCube.SampleLevel(Material.TextureGroupSampler, float3(1.0, 0.0, 0.0), 0.0);
	resourceValue += Material.SceneTexture.Load(int3(0, 0, 0));
	resourceValue += Material.GlobalSDFTexture.Load(int4(0, 0, 0, 0));
	float4 value = mul(float4(Material.Emissive, 1.0), Material.Transform);
	value += Material.Enabled ? Material.Tint : Material.ChannelMask;
	value += Material.BaseColor + Material.GameplayValue + Material.GlobalSDFConstants + resourceValue;
	return value * PerView.ViewTint + (Material.Roughness + Material.LayerIndex) * 0.0002;
}

float4 MaterialOptionalResourcePS(VertexOutput input) : SV_Target
{
	return Material.OptionalTexture.Load(int3(0, 0, 0)) + PerView.ViewTint;
}
