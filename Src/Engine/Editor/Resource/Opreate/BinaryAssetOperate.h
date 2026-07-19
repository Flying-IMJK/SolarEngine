#pragma once
#include "AssetOperate.h"

namespace SE::Editor
{
    /// <summary>
    /// Base class for all binary asset proxy objects used to manage <see cref="BinaryAssetItem"/>.
    /// </summary>
    SE_CLASS(Reflect)
    class BinaryAssetOperate : public AssetOperate
    {
        SE_DEFINE_CLASS_DEFAULT(BinaryAssetOperate, AssetOperate)
    public:
        /// <inheritdoc />
        bool IsProxyFor(ContentItem* item) override;

        /// <inheritdoc />
        bool IsProxyFor(TypeID typeID) override;

        /// <inheritdoc />
        AssetItem* ConstructItem(StringView path, TypeID typeId, UID id) override;

    protected:
        String __GetFileExtension() override;
    };
} // SE

