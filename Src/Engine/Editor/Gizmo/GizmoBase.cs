
using System.Collections.Generic;
using SE.Editor.SceneGraph;

namespace SE.Editor.Gizmo
{
    public abstract class GizmoBase
    {
        public IGizmoOwner Owner { get; private set; }
        public bool IsActive => Owner != null && Owner.Gizmos.Active == this;
        public bool Visible { get; set; } = true;
        public virtual bool IsControllingMouse => false;

        protected GizmoBase(IGizmoOwner owner)
        {
            Owner = owner;
            owner.Gizmos.Add(this);
        }

        public virtual void OnSelectionChanged(List<ScenesGraphNode> selection) { }
        public virtual void OnActivated() { }
        public virtual void OnDeactivated() { }
        public virtual void Update(float deltaTime) { }
        public virtual void Pick() { }
        public virtual void Draw(GPUContext context) { }

        public void Activate()
        {
            Owner.Gizmos.Active = this;
        }

        public virtual void Destroy()
        {
            if (Owner != null)
            {
                Owner.Gizmos.Remove(this);
                Owner = null;
            }
        }
    }
}
