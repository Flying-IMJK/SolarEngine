


namespace SE.Editor
{
    /// <summary>
    /// The main managed editor class. Editor root object.
    /// </summary>
    public sealed partial class Editor
    {
        /// <summary>
        /// Gets the Editor instance.
        /// </summary>
        public static Editor Instance { get; private set; }
        
        
        internal Editor()
        {
            Instance = this;
        }
        
    }
}

