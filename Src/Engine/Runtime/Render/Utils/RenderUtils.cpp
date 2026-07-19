#include "RenderUtils.h"
#include "Runtime/Core/Encoding.h"
#include "../../Utilities/Time.h"
#include "Runtime/Graphics/GPUContext.h"

#include "stb/stb_image.h"
#include "stb/stb_image_resize.h"
#include "Runtime/Graphics/GPUDevice.h"
#include "Runtime/Graphics/Base/GPUUtils.h"
#include "Runtime/Render/RenderContext.h"
#include "Runtime/Render/Assets/Geometry/Model.h"
#include "Runtime/Render/Assets/Geometry/ModelLOD.h"

namespace SE::RenderUtils
{
    bool CreateTextureFromFile(String const& path, GPUTexture*& texture)
    {
        ENGINE_ASSERT(!path.IsEmpty());

        //-------------------------------------------------------------------------

        int32 width, height, channels;
        uint8* pImage = stbi_load(path.ToStringAnsi().Get(), &width, &height, &channels, STBI_rgb_alpha);
        if (pImage == nullptr)
        {
            return false;
        }

		texture = GPUTexture::New();

        // Upload texture to graphics system
		GPUTextureDescription textureDesc = GPUTextureDescription::New2D(width, height, PixelFormat::R8G8B8A8_UNorm);

		texture->Init(textureDesc);
//		GPUContext UpdateTexture()
//        RHISubresourceData textureData;
//        textureData.data_ptr = pImage;
//        textureData.row_pitch = width * channels;
//        textureData.slice_pitch = textureData.row_pitch * height;

        stbi_image_free( pImage );
        return true;
    }

    bool CreateTextureFromBase64(uint8 const* pData, uint64 size, uint32 width, uint32 height, GPUTexture*& texture)
    {
        ENGINE_ASSERT( pData != nullptr && size > 0 );


        List<byte> decodedData;
        Encoding::Base64::Decode(reinterpret_cast<const char*>(pData), size, decodedData);

        //-------------------------------------------------------------------------
        int32 srcWidth, srcHeight, channels;
        uint8* pImage = stbi_load_from_memory(decodedData.Get(), decodedData.Count(), &srcWidth, &srcHeight, &channels, 4 );
        if ( pImage == nullptr )
        {
            return false;
        }

		uint8* pResizeImage = (uint8*)PlatformAllocator::Allocate(size);
		stbir_resize_uint8(pImage, srcWidth, srcHeight, 0, pResizeImage, width, height, 0, 4);

		stbi_image_free( pImage );

		texture = GPUTexture::New();

		// Upload texture to graphics system
		GPUTextureDescription textureDesc = GPUTextureDescription::New2D(width, height, PixelFormat::R8G8B8A8_UNorm, GPUTextureFlags::ShaderResource);

		texture->Init(textureDesc);

		uint32 row_pitch = 0;
		uint32 slice_pitch = 0;
		texture->ComputePitch(0, row_pitch, slice_pitch);
		GPUDevice::instance->GetMainContext()->UpdateTexture(texture, 0, 0, pResizeImage, row_pitch, slice_pitch);
		texture->SetResidentMipLevels(1);

		PlatformAllocator::Free(pResizeImage);

        return true;
    }

    void UpdateModelLODTransition(byte& lodTransition)
    {
    	// TODO: expose this parameter to be configured per game (global config)
	    constexpr float ModelLODTransitionTime = 0.3f;

    	// Update LOD transition (note: LODTransition is mapped from [0-255] to [0-ModelLODTransitionTime] using fixed point value to use 8bit only)
    	const float normalizedProgress = static_cast<float>(lodTransition) * (1.0f / 255.0f);
    	const float deltaProgress = Time::Render.UnscaledDeltaTime.GetTotalSeconds() / ModelLODTransitionTime;
    	const auto newProgress = static_cast<int32>((normalizedProgress + deltaProgress) * 255.0f);
    	lodTransition = static_cast<byte>(Math::Min<int32>(newProgress, 255));
    }

    float ComputeBoundsScreenRadiusSquared(const Float3& origin, const float radius, const RenderView& view)
    {
    	return ComputeBoundsScreenRadiusSquared(origin, radius, view.Position, view.Projection);
    }

    float ComputeBoundsScreenRadiusSquared(const Float3& origin, float radius, const Float3& viewOrigin, const Matrix& projectionMatrix)
    {
    	const float screenMultiple = 0.5f * Math::Max(projectionMatrix.Values[0][0], projectionMatrix.Values[1][1]);
    	const float distSqr = Float3::DistanceSquared(origin, viewOrigin) * projectionMatrix.Values[2][3];
    	return Math::Pow(screenMultiple * radius, 2) / Math::Max(1.0f, distSqr);
    }

    int32 ComputeModelLOD(const Model* model, const Float3& origin, float radius, const RenderContext& renderContext)
    {
    	const auto lodView = (renderContext.lodProxyView ? renderContext.lodProxyView : &renderContext.view);
    	const float screenRadiusSquared = ComputeBoundsScreenRadiusSquared(origin, radius, *lodView) * renderContext.view.ModelLODDistanceFactorSqrt;

    	// Check if model is being culled
    	if (Math::Pow(model->MinScreenSize * 0.5f, 2) > screenRadiusSquared)
    		return -1;

    	// Skip if no need to calculate LOD
    	if (model->LODs.Count() <= 1)
    		return 0;

    	// Iterate backwards and return the first matching LOD
    	for (int32 lodIndex = model->LODs.Count() - 1; lodIndex >= 0; lodIndex--)
    	{
    		if (Math::Pow(model->LODs[lodIndex].ScreenSize * 0.5f, 2) >= screenRadiusSquared)
    		{
    			return lodIndex;
    		}
    	}

    	return 0;
    }

    uint64 CalculateTextureMemoryUsage(PixelFormat format, int32 width, int32 height, int32 mipLevels)
    {
    	uint64 result = 0;

    	if (mipLevels == 0)
    		mipLevels = 69;

    	uint32 rowPitch, slicePitch;
    	while (mipLevels > 0 && (width >= 1 || height >= 1))
    	{
    		GPUUtils::ComputePitch(format, width, height, rowPitch, slicePitch);
    		result += slicePitch;

    		if (width > 1)
    			width >>= 1;
    		if (height > 1)
    			height >>= 1;

    		mipLevels--;
    	}

    	return result;
    }

    void CalculateTangentFrame(FloatR10G10B10A2& resultNormal, FloatR10G10B10A2& resultTangent, const Float3& normal)
    {
    	// Calculate tangent
    	const Float3 c1 = Float3::Cross(normal, Float3::UnitZ);
    	const Float3 c2 = Float3::Cross(normal, Float3::UnitY);
    	const Float3 tangent = c1.LengthSquared() > c2.LengthSquared() ? c1 : c2;

    	// Calculate bitangent sign
    	const Float3 bitangent = Float3::Normalize(Float3::Cross(normal, tangent));
    	const byte sign = static_cast<byte>(Float3::Dot(Float3::Cross(bitangent, normal), tangent) < 0.0f ? 1 : 0);

    	// Set tangent frame
    	resultNormal = Float1010102(normal * 0.5f + 0.5f, 0);
    	resultTangent = Float1010102(tangent * 0.5f + 0.5f, sign);
    }

	void CalculateTangentFrame(FloatR10G10B10A2& resultNormal, FloatR10G10B10A2& resultTangent, const Float3& normal, const Float3& tangent)
    {
    	// Calculate bitangent sign
    	const Float3 bitangent = Float3::Normalize(Float3::Cross(normal, tangent));
    	const byte sign = static_cast<byte>(Float3::Dot(Float3::Cross(bitangent, normal), tangent) < 0.0f ? 1 : 0);

    	// Set tangent frame
    	resultNormal = Float1010102(normal * 0.5f + 0.5f, 0);
    	resultTangent = Float1010102(tangent * 0.5f + 0.5f, sign);
    }

}
