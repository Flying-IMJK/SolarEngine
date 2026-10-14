using SE.GUI;

namespace SE.Editor
{
    [HideInEditor]
    public abstract class LayoutElement
    {
        public abstract Control Control { get; }
    }
}
