
#pragma once

#include "Runtime/Resource/BinaryAsset.h"
#include "Runtime/Graphics/Base/PixelFormat.h"
#include "Runtime/Graphics/Textures/GPUTextureDescription.h"
#include "StreamingTexture.h"

namespace SE
{
	class TextureData;
	class TextureMipData;
	class GPUTexture;

	/// <summary>
	/// The texture init data (external source).
	/// </summary>
	struct SE_API_RUNTIME TextureInitData
	{
		struct MipData
		{
			BytesContainer Data;
			uint32 RowPitch;
			uint32 SlicePitch;

			MipData() = default;
			MipData(MipData&& other) noexcept;
		};

		PixelFormat Format;
		int32 Width;
		int32 Height;
		int32 ArraySize;
		List<MipData, FixedAllocation<14>> Mips;

		TextureInitData() = default;
		TextureInitData(TextureInitData&& other) noexcept;

		TextureInitData& operator=(TextureInitData&& other) noexcept
		{
			if (this != &other)
				*this = MoveTemp(other);
			return *this;
		}

		/// <summary>
		/// Generates the mip map data.
		/// </summary>
		/// <remarks>
		/// Compressed formats are not supported. Point filter supports all types and preserves texture edge values.
		/// </remarks>
		/// <param name="mipIndex">Index of the mip.</param>
		/// <param name="linear">True if use linear filer, otherwise point filtering.</param>
		/// <returns>True if failed, otherwise false.</returns>
		bool GenerateMip(int32 mipIndex, bool linear = false);

		void FromTextureData(const TextureData& textureData, bool generateMips = false);

		static TextureInitData* Create(int32 width, int32 height, int32 arraySize, PixelFormat hormat);
	};

	/// <summary>
	/// Base class for <see cref="Texture"/>, <see cref="SpriteAtlas"/>, <see cref="IESProfile"/> and other assets that can contain texture data.
	/// </summary>
	class SE_API_RUNTIME TextureBase : public BinaryAsset
	{
		friend class StreamingTexture;

		SE_CLASS(TextureBase, BinaryAsset)
	protected:
		StreamingTexture _texture;
		TextureInitData* _customData;
		BinaryAsset* _parent;

	public:
		TextureBase();

		TextureBase(const AssetInfo* info);

		/// <summary>
		/// Gets GPU texture object allocated by the asset.
		/// </summary>
		FORCE_INLINE GPUTexture* GetTexture() const
		{
			return _texture.GetTexture();
		}

		/// <summary>
		/// Gets the texture data format.
		/// </summary>
		FORCE_INLINE PixelFormat Format() const
		{
			return _texture._header.Format;
		}

		/// <summary>
		/// Gets the total width of the texture. Actual resident size may be different due to dynamic content streaming. Returns 0 if texture is not loaded.
		/// </summary>
		FORCE_INLINE int32 Width() const
		{
			return _texture.TotalWidth();
		}

		/// <summary>
		/// Gets the total height of the texture. Actual resident size may be different due to dynamic content streaming. Returns 0 if texture is not loaded.
		/// </summary>
		FORCE_INLINE int32 Height() const
		{
			return _texture.TotalHeight();
		}

		/// <summary>
		/// Gets the total size of the texture. Actual resident size may be different due to dynamic content streaming. Returns Float2::Zero if texture is not loaded.
		/// </summary>
		Float2 Size() const;

		/// <summary>
		/// Gets the total array size of the texture.
		/// </summary>
		int32 GetArraySize() const;

		/// <summary>
		/// Gets the total mip levels count of the texture. Actual resident mipmaps count may be different due to dynamic content streaming.
		/// </summary>
		int32 GetMipLevels() const;

		/// <summary>
		/// Gets the current mip levels count of the texture that are on GPU ready to use.
		/// </summary>
		int32 GetResidentMipLevels() const;

		/// <summary>
		/// Gets the amount of the memory used by this resource. Exact value may differ due to memory alignment and resource allocation policy.
		/// </summary>
		uint64 GetCurrentMemoryUsage() const;

		/// <summary>
		/// Gets the index of the texture group used by this texture.
		/// </summary>
		int32 GetTextureGroup() const;

		/// <summary>
		/// Sets the index of the texture group used by this texture.
		/// </summary>
		void SetTextureGroup(int32 textureGroup);

	public:
		/// <summary>
		/// Gets the mip data.
		/// </summary>
		/// <param name="mipIndex">The mip index (zero-based).</param>
		/// <param name="rowPitch">The data row pitch (in bytes).</param>
		/// <param name="slicePitch">The data slice pitch (in bytes).</param>
		/// <returns>The mip-map data or empty if failed to get it.</returns>
		BytesContainer GetMipData(int32 mipIndex, int32& rowPitch, int32& slicePitch);

		/// <summary>
		/// Loads the texture data from the asset.
		/// </summary>
		/// <param name="result">The result data.</param>
		/// <param name="copyData">True if copy asset data to the result buffer, otherwise texture data will be linked to the internal storage (then the data is valid while asset is loaded and there is no texture data copy operations - faster).</param>
		/// <returns>True if cannot load data, otherwise false.</returns>
		bool GetTextureData(TextureData& result, bool copyData = true);

		/// <summary>
		/// Loads the texture data from the asset (single mip).
		/// </summary>
		/// <param name="result">The result data.</param>
		/// <param name="mipIndex">The mip index (zero-based).</param>
		/// <param name="arrayIndex">The array or depth slice index (zero-based).</param>
		/// <param name="copyData">True if copy asset data to the result buffer, otherwise texture data will be linked to the internal storage (then the data is valid while asset is loaded and there is no texture data copy operations - faster).</param>
		/// <returns>True if cannot load data, otherwise false.</returns>
		bool GetTextureMipData(TextureMipData& result, int32 mipIndex = 0, int32 arrayIndex = 0, bool copyData = true);

		/// <summary>
		/// Gets the texture pixels as Color32 array.
		/// </summary>
		/// <remarks>Supported only for 'basic' texture formats (uncompressed, single plane).</remarks>
		/// <param name="pixels">The result texture pixels array.</param>
		/// <param name="mipIndex">The mip index (zero-based).</param>
		/// <param name="arrayIndex">The array or depth slice index (zero-based).</param>
		/// <returns>True if failed, otherwise false.</returns>
		bool GetPixels(List<Color32>& pixels, int32 mipIndex = 0, int32 arrayIndex = 0);

		/// <summary>
		/// Gets the texture pixels as Color array.
		/// </summary>
		/// <remarks>Supported only for 'basic' texture formats (uncompressed, single plane).</remarks>
		/// <param name="pixels">The result texture pixels array.</param>
		/// <param name="mipIndex">The mip index (zero-based).</param>
		/// <param name="arrayIndex">The array or depth slice index (zero-based).</param>
		/// <returns>True if failed, otherwise false.</returns>
		bool GetPixels(List<Color>& pixels, int32 mipIndex = 0, int32 arrayIndex = 0);

		/// <summary>
		/// Sets the texture pixels as Color32 array (asset must be virtual and already initialized).
		/// </summary>
		/// <remarks>Supported only for 'basic' texture formats (uncompressed, single plane).</remarks>
		/// <param name="pixels">The texture pixels array.</param>
		/// <param name="mipIndex">The mip index (zero-based).</param>
		/// <param name="arrayIndex">The array or depth slice index (zero-based).</param>
		/// <param name="generateMips">Enables automatic mip-maps generation (fast point filter) based on the current mip (will generate lower mips).</param>
		/// <returns>True if failed, otherwise false.</returns>
		bool SetPixels(const Span<Color32>& pixels, int32 mipIndex = 0, int32 arrayIndex = 0, bool generateMips = false);

		/// <summary>
		/// Sets the texture pixels as Color array (asset must be virtual and already initialized).
		/// </summary>
		/// <remarks>Supported only for 'basic' texture formats (uncompressed, single plane).</remarks>
		/// <param name="pixels">The texture pixels array.</param>
		/// <param name="mipIndex">The mip index (zero-based).</param>
		/// <param name="arrayIndex">The array or depth slice index (zero-based).</param>
		/// <param name="generateMips">Enables automatic mip-maps generation (fast point filter) based on the current mip (will generate lower mips).</param>
		/// <returns>True if failed, otherwise false.</returns>
		bool SetPixels(const Span<Color>& pixels, int32 mipIndex = 0, int32 arrayIndex = 0, bool generateMips = false);

		/// <summary>
		/// 使用指定的初始化数据源初始化纹理（资源必须是虚拟的）
		/// </summary>
		/// <param name="initData">The initializer data allocated by the caller with New. It will be owned and released by the asset internal layer.</param>
		bool Init(TextureInitData* initData);

		/// <summary>
		/// 使用指定的初始化数据源初始化纹理（资源必须是虚拟的）
		/// </summary>
		/// <param name="initData">The initializer data. It will be used and released by the asset internal layer (memory allocation will be swapped).</param>
		bool Init(TextureInitData&& initData)
		{
			return Init(New<TextureInitData>(MoveTemp(initData)));
		}

	protected:
		virtual int32 CalculateChunkIndex(int32 mipIndex) const;

	private:
/*#if !COMPILE_WITHOUT_CSHARP
		// Internal bindings
		bool InitCSharp(void* ptr);
#endif*/

	public:
		// [BinaryAsset]
		uint64 GetMemoryUsage() const override;
		void CancelStreaming() override;


		CriticalSection& GetOwnerLocker() const;
		Threading::Task* RequestMipDataAsync(int32 mipIndex);
		Storage::LockData LockData();
		void GetMipData(int32 mipIndex, BytesContainer& data) const;
		void GetMipDataWithLoading(int32 mipIndex, BytesContainer& data) const;
		bool GetMipDataCustomPitch(int32 mipIndex, uint32& rowPitch, uint32& slicePitch) const;

	protected:
		// [BinaryAsset]
		bool OnInit(AssetInitData& initData) override;
		LoadResult load() override;
		void Unload(bool isReloading) override;
	};
}
