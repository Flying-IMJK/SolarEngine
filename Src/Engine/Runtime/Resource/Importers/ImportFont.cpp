
#include "ImportFont.h"

#include "Runtime/Render/2D/FontAsset.h"

namespace SE
{
	CreateAssetResult ImportFont::Import(CreateAssetContext& context)
	{
		// Base
		IMPORT_SETUP(FontAsset, ASSET_VERSION_FONT);

		// Setup header
		FontOptions options;
		options.Hinting = FontHinting::Default;
		options.Flags = FontFlags::AntiAliasing;
		context.Data.CustomData.Copy(&options);

		// Open the file
		auto stream = FileReadStream::Open(context.InputPath);
		if (stream == nullptr)
			return CreateAssetResult::InvalidPath;

		// Copy font file data
		if (!context.AllocateChunk(0))
		{
			Delete(stream);
			return CreateAssetResult::CannotAllocateChunk;
		}

		const auto size = stream->GetLength();
		context.Data.Header.Chunks[0]->Data.Allocate(size);
		stream->ReadBytes(context.Data.Header.Chunks[0]->Get(), size);

		Delete(stream);

		return CreateAssetResult::Ok;
	}

} // SE