using System;
using System.IO;


namespace SE.Editor
{
    public sealed class SceneOperate : AssetOperate
    {
        public const string Extension = "scene";
        public override string Name => "Scene";
        public override string FileExtension => Extension;
        public override Type AssetTypeId => typeof(SceneAsset);

        public override Color AccentColor => Color.FromRGB(0xbb37ef);

        public override bool IsProxyFor(ContentItem item) => item is SceneItem;
        public override bool CanCreate(ContentFolder targetLocation) => targetLocation.CanHaveAssets;

        public override void Create(string outputPath, object argument = null)
        {
            Editor.Instance.Scene.CreateSceneFile(outputPath);
        }

        public override void Open(ContentItem item)
        {
            if (item is SceneItem scene)
            {
                Editor.Instance.Scene.OpenScene(scene.Id);
            }
        }

        public override AssetItem ConstructItem(string path, Type typeId, Guid id)
        {
            return new SceneItem(path, id);
        }
    }
}
