#nullable disable annotations

namespace SE.Editor
{
    /// <summary>
    /// Root tree node for the content workspace.
    /// </summary>
    /// <seealso cref="FlaxEditor.Content.ContentTreeNode" />
    public class RootContentTreeNode : ContentTreeNode
    {
        public RootContentTreeNode()
            : base(null, string.Empty)
        {
        }

        public override string GetNavButtonLabel()
        {
            return " /";
        }
    }
}
