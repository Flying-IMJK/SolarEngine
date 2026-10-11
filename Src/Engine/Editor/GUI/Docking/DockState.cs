namespace SE.Editor.GUI
{
    /// <summary>
    /// Dockable window mode.
    /// </summary>
    public enum DockState
    {
        /// <summary>
        /// The unknown state.
        /// </summary>
        Unknown = 0,

        /// <summary>
        /// The floating window.
        /// </summary>
        Float = 1,

        //DockTopAutoHide = 2,
        //DockLeftAutoHide = 3,
        //DockBottomAutoHide = 4,
        //DockRightAutoHide = 5,

        /// <summary>
        /// The dock fill as a tab.
        /// </summary>
        DockFill = 6,

        /// <summary>
        /// The dock top.
        /// </summary>
        DockTop = 7,

        /// <summary>
        /// The dock left.
        /// </summary>
        DockLeft = 8,

        /// <summary>
        /// The dock bottom.
        /// </summary>
        DockBottom = 9,

        /// <summary>
        /// The dock right.
        /// </summary>
        DockRight = 10,

        /// <summary>
        /// The hidden state.
        /// </summary>
        Hidden = 11
    }
}
