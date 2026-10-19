// Copyright (c) Wojciech Figat. All rights reserved.

using SE.GUI;

namespace SE.Editor
{
    /// <summary>
    /// The custom layout element.
    /// </summary>
    /// <seealso cref="SE.Editor.LayoutElementsContainer" />
    public class CustomElementsContainer<T> : LayoutElementsContainer
    where T : ContainerControl, new()
    {
        /// <summary>
        /// The custom control.
        /// </summary>
        public readonly T CustomControl = new T();

        /// <inheritdoc />
        public override ContainerControl ContainerControl => CustomControl;
    }

    /// <summary>
    /// The custom layout element.
    /// </summary>
    /// <seealso cref="SE.Editor.LayoutElement" />
    public class CustomElement<T> : LayoutElement
    where T : Control, new()
    {
        /// <summary>
        /// The custom control.
        /// </summary>
        public readonly T CustomControl = new T();

        /// <inheritdoc />
        public override Control Control => CustomControl;
    }
}
