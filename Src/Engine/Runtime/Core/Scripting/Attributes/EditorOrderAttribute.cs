using System;

namespace SE
{
    /// <summary>
    /// Controls an item's order in the editor.
    /// </summary>
    [Serializable]
    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property | AttributeTargets.Delegate | AttributeTargets.Event | AttributeTargets.Method)]
    public sealed class EditorOrderAttribute : Attribute
    {
        /// <summary>
        /// The requested editor layout order.
        /// </summary>
        public int Order;

        private EditorOrderAttribute()
        {
        }

        /// <summary>
        /// Initializes a new editor order attribute.
        /// </summary>
        public EditorOrderAttribute(int order)
        {
            Order = order;
        }
    }
}
