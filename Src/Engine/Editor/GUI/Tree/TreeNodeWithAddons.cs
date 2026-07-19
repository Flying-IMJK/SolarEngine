using System;
using System.Collections.Generic;
using SE.GUI;

namespace SE.Editor.GUI
{
    /// <summary>
    /// A tree node that hosts additional controls in its header area.
    /// </summary>
    public class TreeNodeWithAddons : TreeNode
    {
        private readonly List<Control> _addons = new();

        public TreeNodeWithAddons(bool canChangeOrder = false)
            : base(canChangeOrder)
        {
        }

        public IReadOnlyList<Control> Addons => _addons;

        /// <summary>
        /// Adds a header addon. Its bounds are relative to the tree-node header.
        /// </summary>
        public T AddAddon<T>(T addon) where T : Control
        {
            ArgumentNullException.ThrowIfNull(addon);
            if (_addons.Contains(addon))
                return addon;
            _addons.Add(addon);
            AddChild(addon);
            return addon;
        }

        public bool RemoveAddon(Control addon)
        {
            ArgumentNullException.ThrowIfNull(addon);
            if (!_addons.Remove(addon))
                return false;
            return RemoveChild(addon);
        }
    }
}
