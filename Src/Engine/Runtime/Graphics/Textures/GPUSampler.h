#pragma once

#include "Runtime/Graphics/Base/GPUResource.h"
#include "GPUSamplerDescription.h"

namespace SE
{

	/// <summary>
	/// GPU texture sampler object.
	/// </summary>
	/// <seealso cref="GPUResource" />
	SE_CLASS(API, Sealed)
	class SE_API_RUNTIME GPUSampler : public GPUResource
	{
		SCRIPTING_TYPE_NO_SPAWN(GPUSampler);

	public:
		static GPUSampler* Spawn(const SpawnParams& params);
		static GPUSampler* New();

	protected:
		GPUSamplerDescription m_Desc;

		GPUSampler();

	public:
		/// <summary>
		/// Gets sampler description structure.
		/// </summary>
		const GPUSamplerDescription& GetDescription() const
		{
			return m_Desc;
		}

	public:
		/// <summary>
		/// Creates new sampler.
		/// </summary>
		/// <param name="desc">The sampler description.</param>
		/// <returns>True if cannot create sampler, otherwise false.</returns>
		bool Init(const GPUSamplerDescription& desc);

		// [GPUResource]
        String ToString() const override;
		GPUResourceType GetResType() const final override;

	protected:
		virtual bool OnInit() = 0;
		// [GPUResource]
		void OnReleaseGPU() override;

	};

}
