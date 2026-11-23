using System;
using System.IO;
using SE.Editor.GUI;

namespace SE.Editor.SceneGraph
{
    /// <summary>
    /// Scene graph node for a loaded runtime scene.
    /// </summary>
    public sealed class SceneGraphNode : ActorGraphNode
    {
        private bool m_IsEdited;

        public SceneGraphNode(ScenesGraphFactory factory, SE.Scene scene)
            : base(factory, scene, scene.SceneObjectId, new SceneTreeNode())
        {
        }

        public bool IsEdited
        {
            get => m_IsEdited;
            set
            {
                if (m_IsEdited == value)
                {
                    return;
                }

                m_IsEdited = value;
                TreeNode.UpdateText();
            }
        }

        public SE.Scene Scene => (SE.Scene)Actor!;

        public override bool CanDelete => false;
        public override bool CanDuplicate => false;
        public override bool CanCopyPaste => false;
        public override bool CanDrag => false;
        public override SceneGraphNode ParentScene => this;

        public override void OnContextMenu(ContextMenu contextMenu, EditorWindow window)
        {
            if (IsDisposed)
            {
                return;
            }
            Editor editor = Editor.Instance;
            contextMenu.AddSeparator();
            string path = Scene.Path;
            if (!string.IsNullOrEmpty(path) && File.Exists(path))
            {
                ContentItem item = editor.AssetDatabase.Find(path);
                ContextMenuButton locate = contextMenu.AddButton("Show in content window", () => editor.Windows.Content.Select(item));
                locate.Enabled = item != null;
            }
            ContextMenuButton save = contextMenu.AddButton("Save scene", () => editor.Scene.SaveScene(this));
            save.Enabled = IsEdited;
            contextMenu.AddButton("Unload scene", () => editor.Scene.CloseScene(Scene));
            if (SE.Level.ScenesCount > 1)
            {
                contextMenu.AddButton("Unload all but this scene", () => editor.Scene.CloseAllScenesExcept(Scene));
            }
            base.OnContextMenu(contextMenu, window);
        }
    }
}
