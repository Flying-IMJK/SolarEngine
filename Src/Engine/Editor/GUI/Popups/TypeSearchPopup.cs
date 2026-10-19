// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Reflection;
using SE.GUI;
using CategoryAttribute = System.ComponentModel.CategoryAttribute;

namespace SE.Editor.GUI
{
    /// <summary>
    /// Popup that shows the list of types to pick. Supports searching and basic type filtering.
    /// </summary>
    /// <seealso cref="SE.Editor.GUI.ItemsListContextMenu" />
    public class TypeSearchPopup : ItemsListContextMenu
    {
        /// <summary>
        /// The type item.
        /// </summary>
        /// <seealso cref="SE.Editor.GUI.ItemsListContextMenu.Item" />
        public class TypeItemView : Item
        {
            private Type m_Type;

            /// <summary>
            /// Gets the type.
            /// </summary>
            public Type Type => m_Type;

            /// <summary>
            /// Initializes a new instance of the <see cref="TypeItemView"/> class.
            /// </summary>
            public TypeItemView()
            {
                m_Type = null;
                Name = "<null>";
                TooltipText = "Unset value.";
                Tag = m_Type;
            }

            /// <summary>
            /// Initializes a new instance of the <see cref="TypeItemView"/> class.
            /// </summary>
            /// <param name="type">The type.</param>
            public TypeItemView(Type type)
            : this(type, type.GetCustomAttributes(false))
            {
            }

            /// <summary>
            /// Initializes a new instance of the <see cref="TypeItemView"/> class.
            /// </summary>
            /// <param name="type">The type.</param>
            /// <param name="attributes">The type attributes.</param>
            public TypeItemView(Type type, object[] attributes)
            {
                m_Type = type;

                Name = type.Name;
                TooltipText = attributes.OfType<TooltipAttribute>().FirstOrDefault()?.Text ??
                    attributes.OfType<DescriptionAttribute>().FirstOrDefault()?.Description;
                Tag = type;
                CategoryAttribute categoryAttribute =
                    (CategoryAttribute)attributes.FirstOrDefault(x => x is CategoryAttribute);
                if (categoryAttribute != null)
                {
                    Category = categoryAttribute.Category;
                }
            }

            /// <inheritdoc />
            public override void OnDestroy()
            {
                m_Type = null;

                base.OnDestroy();
            }
        }

        /// <summary>
        /// Validates if the given actor item can be used to pick it.
        /// </summary>
        /// <param name="type">The type.</param>
        /// <returns>True if is valid.</returns>
        public delegate bool IsValidDelegate(Type type);

        private IsValidDelegate m_IsValid;
        private Action<Type> m_Selected;

        private TypeSearchPopup(IsValidDelegate isValid, Action<Type> selected)
        {
            m_IsValid = isValid;
            m_Selected = selected;

            ItemClicked += OnItemClicked;

            // TODO: use async thread to search types without UI stall
            List<Type> allTypes = GetTypes().ToList();
            AddItem(new TypeItemView());
            for (int i = 0; i < allTypes.Count; i++)
            {
                Type type = allTypes[i];
                if (m_IsValid(type))
                {
                    object[] attributes = type.GetCustomAttributes(true);
                    if (IsHideAttributes(attributes))
                    {
                        Type mType = type;
                        if (mType != null)
                        {
                            // Skip if type is compiler-generated
                            if (mType.IsValueType && mType.ReflectedType != null &&
                                string.Equals(mType.ReflectedType.Name, "<PrivateImplementationDetails>",
                                    StringComparison.Ordinal))
                            {
                                continue;
                            }

                            // Skip if outer type is hidden
                            if (mType.DeclaringType != null &&
                                IsHideAttributes(mType.DeclaringType.GetCustomAttributes(true)))
                            {
                                continue;
                            }

                            // Blacklist some types
                            if (typeof(TypeConverter).IsAssignableFrom(mType) ||
                                (mType.Namespace != null && mType.Namespace.StartsWith("Newtonsoft.Json")))
                            {
                                continue;
                            }
                        }

                        AddItem(new TypeItemView(type, attributes));
                    }
                }
            }
            SortItems();
        }

        private TypeSearchPopup(IEnumerable<Type> items, Action<Type> selected)
        {
            m_IsValid = null;
            m_Selected = selected;

            ItemClicked += OnItemClicked;

            foreach (Type item in items)
            {
                AddItem(new TypeItemView(item));
            }
            SortItems();
        }

        // Solar exposes loaded CLR assemblies instead of Flax's CodeEditing type cache.
        internal static IEnumerable<Type> GetTypes()
        {
            // Match CodeEditing.All's explicit standard-library entries and assembly boundary.
            Type[] builtInTypes =
            {
                typeof(void), typeof(bool), typeof(byte), typeof(short), typeof(ushort), typeof(int), typeof(uint),
                typeof(long), typeof(ulong), typeof(float), typeof(double), typeof(string), typeof(Guid),
                typeof(DateTime), typeof(TimeSpan),
            };
            foreach (Type type in builtInTypes)
            {
                yield return type;
            }
            foreach (Assembly assembly in Interop.NativeInteropUtils.GetAssemblies())
            {
                if (!HasAssemblyValidAnyTypes(assembly))
                {
                    continue;
                }
                Type[] types;
                try
                {
                    types = assembly.GetTypes();
                }
                catch (ReflectionTypeLoadException ex)
                {
                    types = ex.Types;
                }
                foreach (Type type in types)
                {
                    if (type != null)
                    {
                        yield return type;
                    }
                }
            }
        }

        private static bool HasAssemblyValidAnyTypes(Assembly assembly)
        {
            if (string.IsNullOrEmpty(assembly.Location))
            {
                return true;
            }
            if (assembly.ManifestModule.FullyQualifiedName == "<In Memory Module>")
            {
                return false;
            }
            string repositoryUrl = assembly.GetCustomAttributes<AssemblyMetadataAttribute>()
                .FirstOrDefault(x => x.Key == "RepositoryUrl")?.Value ?? string.Empty;
            return repositoryUrl != "https://github.com/dotnet/runtime";
        }

        private bool IsHideAttributes(object[] attributes)
        {
            return attributes.FirstOrDefault(IsHideAttribute) == null;
        }

        private bool IsHideAttribute(object attr)
        {
            return attr is HideInEditorAttribute ||
                   attr is System.Runtime.CompilerServices.CompilerGeneratedAttribute;
        }

        private void OnItemClicked(Item item)
        {
            m_Selected(((TypeItemView)item).Type);
        }

        /// <summary>
        /// Shows the popup.
        /// </summary>
        /// <param name="showTarget">The show target.</param>
        /// <param name="showTargetLocation">The show target location.</param>
        /// <param name="isValid">Event called to check if a given asset item is valid to be used.</param>
        /// <param name="selected">Event called on asset item pick.</param>
        /// <returns>The dialog.</returns>
        public static TypeSearchPopup Show(Control showTarget, Float2 showTargetLocation, IsValidDelegate isValid,
            Action<Type> selected)
        {
            TypeSearchPopup popup = new TypeSearchPopup(isValid, selected);
            popup.Show(showTarget, showTargetLocation);
            return popup;
        }

        /// <summary>
        /// Shows the popup.
        /// </summary>
        /// <param name="showTarget">The show target.</param>
        /// <param name="showTargetLocation">The show target location.</param>
        /// <param name="items">Collection of types to display available to pick.</param>
        /// <param name="selected">Event called on asset item pick.</param>
        /// <returns>The dialog.</returns>
        public static TypeSearchPopup Show(Control showTarget, Float2 showTargetLocation, IEnumerable<Type> items,
            Action<Type> selected)
        {
            TypeSearchPopup popup = new TypeSearchPopup(items, selected);
            popup.Show(showTarget, showTargetLocation);
            return popup;
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            m_IsValid = null;
            m_Selected = null;

            base.OnDestroy();
        }
    }
}
