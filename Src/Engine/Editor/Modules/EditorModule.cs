using System;

namespace SE.Editor
{
    /// <summary>
    /// Base class for managed editor modules. The <see cref="Editor"/> owns the
    /// lifecycle and calls the protected hooks in a stable dependency order.
    /// </summary>
    public abstract class EditorModule
    {
        protected EditorModule(Editor editor)
        {
            Editor = editor ?? throw new ArgumentNullException(nameof(editor));
        }

        protected Editor Editor { get; }
        
        internal virtual int Order => 0;
        

        public virtual void OnInit()
        {
        }

        public virtual void OnEndInit()
        {
        }

        public virtual void OnUpdate()
        {
        }

        public virtual void OnLateUpdate()
        {
        }

        public virtual void OnRender()
        {
        }

        public virtual void OnDispose()
        {
        }
    }
}
