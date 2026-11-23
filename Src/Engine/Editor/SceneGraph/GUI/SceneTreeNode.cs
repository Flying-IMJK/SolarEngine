
using SE;
using SE.Editor.GUI;
using SE.GUI;

namespace SE.Editor.SceneGraph
{
    /// <summary>
    /// A <see cref="ActorTreeNode"/> custom implementation for <see cref="SceneNode"/>.
    /// </summary>
    /// <seealso cref="SE.Editor.GUI.ActorTreeNode" />
    public sealed class SceneTreeNode : ActorTreeNode
    {

        public override void UpdateText()
        {
            base.UpdateText();

            // Append asset name if used
            var filename = ((Scene)m_ActorNode.Actor).Filename;
            if (!string.IsNullOrEmpty(filename))
            {
                Text += " (";
                Text += filename;
                Text += ")";
            }

            // Append star character to modified scenes
            if (m_ActorNode is SceneGraphNode node && node.IsEdited)
            {
                Text += "*";
            }
        }

        protected override Color CacheTextColor()
        {
            return Style.Current.Foreground;
        }
    }
}
