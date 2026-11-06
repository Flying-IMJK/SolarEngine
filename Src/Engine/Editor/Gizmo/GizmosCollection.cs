

using System;
using System.Collections.Generic;

namespace SE.Editor.Gizmo
{
    public class GizmosCollection : List<GizmoBase>
    {
        private readonly IGizmoOwner m_Owner;
        private readonly List<EditorGizmoMode> m_Modes = new();
        private GizmoBase m_Active;
        private EditorGizmoMode m_ActiveMode;

        public event Action ActiveChanged;
        public event Action<EditorGizmoMode> ActiveModeChanged;

        public GizmosCollection(IGizmoOwner owner)
        {
            m_Owner = owner;
        }

        public GizmoBase Active
        {
            get => m_Active;
            set
            {
                if (m_Active == value)
                {
                    return;
                }
                if (value != null && !Contains(value))
                {
                    throw new ArgumentException("Gizmo has not been added to this viewport.");
                }
                m_Active?.OnDeactivated();
                m_Active = value;
                m_Active?.OnActivated();
                ActiveChanged?.Invoke();
            }
        }

        public EditorGizmoMode ActiveMode
        {
            get => m_ActiveMode;
            set
            {
                if (m_ActiveMode == value)
                {
                    return;
                }
                if (value != null && (!m_Modes.Contains(value) || value.Owner != m_Owner))
                {
                    throw new ArgumentException("Mode has not been added to this viewport.");
                }
                m_ActiveMode?.OnDeactivated();
                Active = null;
                m_ActiveMode = value;
                m_ActiveMode?.OnActivated();
                ActiveModeChanged?.Invoke(value);
            }
        }

        public new void Remove(GizmoBase item)
        {
            if (Active == item)
            {
                Active = null;
            }
            base.Remove(item);
        }

        public new void Clear()
        {
            ActiveMode = null;
            Active = null;
            foreach (EditorGizmoMode mode in m_Modes)
            {
                mode.Dispose();
            }
            m_Modes.Clear();
            while (Count != 0)
            {
                this[Count - 1].Destroy();
            }
        }

        public void AddMode(EditorGizmoMode mode)
        {
            ArgumentNullException.ThrowIfNull(mode);
            if (m_Modes.Contains(mode) || mode.Owner != null)
            {
                throw new ArgumentException("Mode already has an owner.");
            }
            m_Modes.Add(mode);
            mode.Init(m_Owner);
        }

        public void RemoveMode(EditorGizmoMode mode)
        {
            ArgumentNullException.ThrowIfNull(mode);
            if (!m_Modes.Contains(mode))
            {
                throw new ArgumentException("Mode has not been added to this viewport.");
            }
            if (ActiveMode == mode)
            {
                ActiveMode = null;
            }
            m_Modes.Remove(mode);
            mode.Dispose();
        }

        public T SetActiveMode<T>() where T : EditorGizmoMode
        {
            foreach (EditorGizmoMode mode in m_Modes)
            {
                if (mode is T result)
                {
                    ActiveMode = result;
                    return result;
                }
            }
            throw new ArgumentException("Mode has not been added to this viewport.");
        }

        public T Get<T>() where T : GizmoBase
        {
            foreach (GizmoBase gizmo in this)
            {
                if (gizmo is T result)
                {
                    return result;
                }
            }
            return null;
        }
    }
}
