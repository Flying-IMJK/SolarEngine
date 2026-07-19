using SE.GUI;
namespace SE.Editor.GUI
{
    /// <summary>
    /// Editor viewport specialization of the Runtime render-output control.
    /// </summary>
    public class RenderOutputViewport : RenderOutputControl
    {
        public RenderOutputViewport(object renderTask)
            : base(renderTask)
        {
            SetBounds(0, 0, 320, 180);
        }
    }
}
