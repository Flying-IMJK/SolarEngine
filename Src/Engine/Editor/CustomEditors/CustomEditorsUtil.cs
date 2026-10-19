using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Runtime.Loader;
using System.Runtime.CompilerServices;

namespace SE.Editor
{
    internal static class CustomEditorsUtil
    {
        private sealed class Entry
        {
            public Type Default;
            public Type Custom;
        }
        private sealed class ContextState { public bool Unloading; }
        private static readonly object RegistryLock = new();
        private static readonly Dictionary<Assembly, Dictionary<Type, Entry>> Registry = new();
        private static readonly HashSet<Assembly> Pending = new();
        private static readonly ConditionalWeakTable<AssemblyLoadContext, ContextState> Contexts = new();

        static CustomEditorsUtil()
        {
            AppDomain.CurrentDomain.AssemblyLoad += (_, args) => QueueAssembly(args.LoadedAssembly);
            foreach (Assembly assembly in AppDomain.CurrentDomain.GetAssemblies())
            {
                QueueAssembly(assembly);
            }
        }

        private static void QueueAssembly(Assembly assembly)
        {
            lock (RegistryLock)
            {
                AssemblyLoadContext context = AssemblyLoadContext.GetLoadContext(assembly);
                if (context != null)
                {
                    if (!Contexts.TryGetValue(context, out ContextState state))
                    {
                        state = new ContextState();
                        Contexts.Add(context, state);
                        context.Unloading += OnUnloading;
                    }
                    if (state.Unloading)
                    {
                        return;
                    }
                }
                if (!Registry.ContainsKey(assembly))
                {
                    Pending.Add(assembly);
                }
            }
        }

        private static void OnUnloading(AssemblyLoadContext context)
        {
            lock (RegistryLock)
            {
                if (Contexts.TryGetValue(context, out ContextState state))
                {
                    state.Unloading = true;
                }
                Pending.RemoveWhere(a => AssemblyLoadContext.GetLoadContext(a) == context);
                foreach (Assembly assembly in Registry.Keys.ToArray())
                {
                    if (AssemblyLoadContext.GetLoadContext(assembly) == context)
                    {
                        Registry.Remove(assembly);
                    }
                    else
                    {
                        Dictionary<Type, Entry> entries = Registry[assembly];
                        foreach (Type type in entries.Keys.ToArray())
                        {
                            if (AssemblyLoadContext.GetLoadContext(type.Assembly) == context)
                            {
                                entries.Remove(type);
                            }
                        }
                    }
                }
            }
        }

        private static void ScanPendingAssemblies()
        {
            // Called under RegistryLock. Reflection may queue more assemblies while resolving attributes.
            while (Pending.Count != 0)
            {
                Assembly assembly = Pending.First();
                Pending.Remove(assembly);
                Dictionary<Type, Entry> entries = new Dictionary<Type, Entry>();
                Registry.Add(assembly, entries);
                Type[] types;
                try { types = assembly.GetTypes(); }
                catch (ReflectionTypeLoadException ex) { types = ex.Types.OfType<Type>().ToArray(); }
                foreach (Type editor in types.OrderBy(t => t.FullName, StringComparer.Ordinal))
                {
                    if (editor.IsAbstract || editor.ContainsGenericParameters || !typeof(CustomEditor)
                        .IsAssignableFrom(editor))
                    {
                        continue;
                    }
                    CustomEditorAttribute attribute = editor.GetCustomAttribute<CustomEditorAttribute>(false);
                    if (attribute == null)
                    {
                        continue;
                    }
                    if (!entries.TryGetValue(attribute.Type, out Entry entry))
                    {
                        entries.Add(attribute.Type, entry = new Entry());
                    }
                    if (editor.IsDefined(typeof(DefaultEditorAttribute), false))
                    {
                        entry.Default ??= editor;
                    }
                    else
                    {
                        entry.Custom ??= editor;
                    }
                }
            }
        }

        internal static CustomEditor CreateEditor(ValueContainer values, CustomEditor overrideEditor = null,
            bool canUseRefPicker = true)
        {
            if (overrideEditor != null)
            {
                return overrideEditor;
            }
            Type type = values.Type;
            if (type == typeof(object) && values.Count > 0 && values[0] != null && !values.HasDifferentTypes)
            {
                type = values[0]!.GetType();
            }
            if (type.IsArray && type.GetArrayRank() != 1)
            {
                return new GenericEditor();
            }
            if (canUseRefPicker)
            {
                if (type.IsInterface && values.Count > 0 && values[0] is SE.Object)
                {
                    return new ObjectRefEditor();
                }
                if (typeof(Asset).IsAssignableFrom(type))
                {
                    return new AssetRefEditor();
                }
                if (typeof(GPUTexture).IsAssignableFrom(type))
                {
                    return new GPUTextureEditor();
                }
                if (typeof(SE.Object).IsAssignableFrom(type))
                {
                    return new ObjectRefEditor();
                }
            }
            Type checkType = type;
            while (checkType != null)
            {
                Type customEditorType = FindEditorForType(checkType);
                if (customEditorType != null)
                {
                    return (CustomEditor)Activator.CreateInstance(customEditorType)!;
                }
                checkType = checkType.BaseType;
                if (!canUseRefPicker && checkType == typeof(SE.Object))
                {
                    break;
                }
            }
            if (type.GetCustomAttribute<CustomEditorAttribute>() is { } customAttribute)
            {
                return (CustomEditor)Activator.CreateInstance(customAttribute.Type)!;
            }
            if (type == typeof(bool) || type == typeof(string) || type.IsEnum || IsNumber(type))
            {
                return new BasicValueEditor();
            }
            if (type.IsGenericType && FindEditorForType(type.GetGenericTypeDefinition()) is { } genericEditorType)
            {
                return (CustomEditor)Activator.CreateInstance(genericEditorType)!;
            }
            return new GenericEditor();
        }

        internal static Type ResolveEditorAlias(string name)
        {
            lock (RegistryLock)
            {
                ScanPendingAssemblies();
                return Registry.Keys.OrderBy(a => a.FullName, StringComparer.Ordinal)
                    .Select(a => a.GetType(name, false)).FirstOrDefault(t => t != null) ?? Type.GetType(name);
            }
        }

        private static Type FindEditorForType(Type targetType)
        {
            if (targetType.GetCustomAttribute<CustomEditorAttribute>(false) is { } declaration)
            {
                return declaration.Type;
            }
            lock (RegistryLock)
            {
                ScanPendingAssemblies();
                Type fallback = null;
                foreach (KeyValuePair<Assembly, Dictionary<Type, Entry>> assembly in Registry.OrderBy(a => a.Key
                    .FullName, StringComparer.Ordinal))
                {
                    if (!assembly.Value.TryGetValue(targetType, out Entry entry))
                    {
                        continue;
                    }
                    if (entry.Custom != null)
                    {
                        return entry.Custom;
                    }
                    fallback ??= entry.Default;
                }
                return fallback;
            }
        }

        private static bool IsNumber(Type type)
        {
            return Type.GetTypeCode(type) is
                        TypeCode.Byte or TypeCode.SByte or TypeCode.Int16 or TypeCode.UInt16 or
                        TypeCode.Int32 or TypeCode.UInt32 or TypeCode.Int64 or TypeCode.UInt64 or
                        TypeCode.Single or TypeCode.Double or TypeCode.Decimal;
        }
    }
}
