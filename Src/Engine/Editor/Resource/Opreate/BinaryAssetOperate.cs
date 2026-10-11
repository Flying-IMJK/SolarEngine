using System;

namespace SE.Editor
{
    public abstract class BinaryAssetOperate : AssetOperate
    {
        public override AssetItem ConstructItem(string path, Type typeId, Guid id)
        {
            return new BinaryAssetItem(path, id, typeId, ContentItemSearchFilter.Other);
        }
    }
}
