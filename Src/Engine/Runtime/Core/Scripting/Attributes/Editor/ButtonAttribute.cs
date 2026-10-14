using System;

namespace SE
{
    /// <summary>
    /// Displays a parameterless method as a button in the properties panel.
    /// </summary>
    [AttributeUsage(AttributeTargets.Method)]
    public sealed class ButtonAttribute : Attribute
    {
        public string? Text;
        public string? Tooltip;

        public ButtonAttribute()
        {
        }

        public ButtonAttribute(string text, string? tooltip = null)
        {
            Text = text;
            Tooltip = tooltip;
        }
    }
}
