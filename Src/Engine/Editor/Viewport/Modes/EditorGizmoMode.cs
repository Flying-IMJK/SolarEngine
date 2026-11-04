// Adapted from Flax Editor. Copyright (c) Wojciech Figat. All rights reserved.

using System;
using SE.Editor.Gizmo;

namespace SE.Editor
{
    public abstract class EditorGizmoMode
    {
        public IGizmoOwner Owner { get; private set; }
        public event Action Activated;
        public event Action Deactivated;

        public virtual void Init(IGizmoOwner owner)
        {
            Owner = owner;
        }

        public virtual void Dispose()
        {
            Owner = null;
        }

        public virtual void OnActivated() => Activated?.Invoke();
        public virtual void OnDeactivated() => Deactivated?.Invoke();
    }

    public sealed class TransformGizmoMode : EditorGizmoMode
    {
        public override void OnActivated()
        {
            base.OnActivated();
            Owner.Gizmos.Active = Owner.Gizmos.Get<TransformGizmo>();
        }
    }
}
