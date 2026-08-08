#include "ResourceInterop.h"

#include "Runtime/Resource/Importers/AssetsImportingSystem.h"
#include "Runtime/Resource/Importers/ImportModel.h"
#include "Runtime/Utilities/Texture/TextureUtils.h"

namespace
{
    bool CanImport(const SE::Char* inputPath, const SE::Char* outputPath)
    {
        return inputPath != nullptr && inputPath[0] != 0 && outputPath != nullptr && outputPath[0] != 0;
    }
}

extern "C" int32_t ResourceInterop_ImportTexture(const SE::Char* inputPath, const SE::Char* outputPath)
{
    if (!CanImport(inputPath, outputPath))
    {
        return 0;
    }

    SE::TextureUtils::Options options;
    const bool failed = SE::AssetsImporting::Import(SE::StringView(inputPath), SE::StringView(outputPath), &options);
    return failed ? 0 : 1;
}

extern "C" int32_t ResourceInterop_ImportModel(const SE::Char* inputPath, const SE::Char* outputPath)
{
    if (!CanImport(inputPath, outputPath))
    {
        return 0;
    }

    SE::ImportModel::Options options;
    const bool failed = SE::AssetsImporting::Import(SE::StringView(inputPath), SE::StringView(outputPath), &options);
    return failed ? 0 : 1;
}
