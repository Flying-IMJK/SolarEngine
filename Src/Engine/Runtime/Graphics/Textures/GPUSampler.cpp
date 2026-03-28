
#include "GPUSampler.h"
#include "GPUSamplerDescription.h"

namespace SE
{
	// GPUSampler
	GPUSampler* GPUSampler::New()
	{
		return nullptr;
	}

	GPUSampler::GPUSampler() : GPUResource()
	{

	}

	bool GPUSampler::Init(const GPUSamplerDescription& desc)
	{
		ReleaseGPU();
		m_Desc = desc;
		if (!OnInit())
		{
			ReleaseGPU();
			LOG_WARNING("Graphic", "GPUSampler Cannot initialize sampler. Description: {0}", desc.ToString());
			return false;
		}
		return true;
	}

	String GPUSampler::ToString() const
	{
		return String();
	}

	GPUResourceType GPUSampler::GetResType() const
	{
		return GPUResourceType::Sampler;
	}

	void GPUSampler::OnReleaseGPU()
	{
		m_Desc.Clear();
	}


	// GPUSamplerDescription

	GPUSamplerDescription GPUSamplerDescription::New(GPUSamplerFilter filter, GPUSamplerAddressMode addressMode)
	{
		GPUSamplerDescription desc;
		desc.Clear();
		desc.Filter = filter;
		desc.AddressU = desc.AddressV = desc.AddressW = addressMode;
		return desc;
	}

	void GPUSamplerDescription::Clear()
	{
		Platform::MemoryClear(this, sizeof(GPUSamplerDescription));
		MaxMipLevel = 1000;
		MinMipLevel = -1000;
	}

	String GPUSamplerDescription::ToString() const
	{
		return String::Format(SE_TEXT("Filter: {}, Address: {}x{}x{}, MipBias: {}, MaxAnisotropy: {}, MinMipLevel: {}, MaxMipLevel: {}, BorderColor: {}, ComparisonFunction: {}"),
			(int32)Filter,
			(int32)AddressU,
			(int32)AddressV,
			(int32)AddressW,
			MipBias,
			MaxAnisotropy,
			MinMipLevel,
			MaxMipLevel,
			(int32)BorderColor,
			(int32)ComparisonFunction);
	}

	bool GPUSamplerDescription::Equals(const GPUSamplerDescription& other) const
	{
		return Filter == other.Filter
			&& AddressU == other.AddressU
			&& AddressV == other.AddressV
			&& AddressW == other.AddressW
			&& MipBias == other.MipBias
			&& MaxAnisotropy == other.MaxAnisotropy
			&& MinMipLevel == other.MinMipLevel
			&& MaxMipLevel == other.MaxMipLevel
			&& BorderColor == other.BorderColor
			&& ComparisonFunction == other.ComparisonFunction;
	}

	uint32 GetHash(const GPUSamplerDescription& key)
	{
		uint32 hashCode = (uint32)key.Filter;
		hashCode = (hashCode * 397) ^ (uint32)key.AddressU;
		hashCode = (hashCode * 397) ^ (uint32)key.AddressV;
		hashCode = (hashCode * 397) ^ (uint32)key.AddressW;
		hashCode = (hashCode * 397) ^ (uint32)key.MipBias;
		hashCode = (hashCode * 397) ^ (uint32)key.MaxAnisotropy;
		hashCode = (hashCode * 397) ^ (uint32)key.MinMipLevel;
		hashCode = (hashCode * 397) ^ (uint32)key.MaxMipLevel;
		hashCode = (hashCode * 397) ^ (uint32)key.BorderColor;
		hashCode = (hashCode * 397) ^ (uint32)key.ComparisonFunction;
		return hashCode;
	}
}