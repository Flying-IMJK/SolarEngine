using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;

namespace SE.Editor
{
    internal static class CustomEditorsUtil
    {
        private static readonly Dictionary<Type, Type?> EditorTypes = new();

        static CustomEditorsUtil()
        {
            AppDomain.CurrentDomain.AssemblyLoad += (_, _) => EditorTypes.Clear();
        }

        internal static CustomEditor CreateEditor(ValueContainer values, CustomEditor? overrideEditor = null)
        {
            if (overrideEditor != null)
                return overrideEditor;
            Type type = values.Type;
            if (type == typeof(object) && values.Count > 0 && values[0] != null && !values.HasDifferentTypes)
                type = values[0]!.GetType();
            if (type.IsArray && type.GetArrayRank() != 1)
                return new GenericEditor();
            Type? checkType = type;
            while (checkType != null)
            {
                Type? customEditorType = FindEditorForType(checkType);
                if (customEditorType != null)
                    return (CustomEditor)Activator.CreateInstance(customEditorType)!;
                checkType = checkType.BaseType;
            }
            if (type.GetCustomAttribute<CustomEditorAttribute>() is { } customAttribute)
                return (CustomEditor)Activator.CreateInstance(customAttribute.Type)!;
            if (type == typeof(bool) || type == typeof(string) || type.IsEnum || IsNumber(type))
                return new BasicValueEditor();
            if (type.IsGenericType && FindEditorForType(type.GetGenericTypeDefinition()) is { } genericEditorType)
                return (CustomEditor)Activator.CreateInstance(genericEditorType)!;
            return new GenericEditor();
        }

        internal static Type? ResolveEditorAlias(string name)
        {
            Type? type = Type.GetType(name);
            if (type != null)
                return type;
            return AppDomain.CurrentDomain.GetAssemblies()
                .Select(assembly => assembly.GetType(name, false))
                .FirstOrDefault(candidate => candidate != null);
        }

        private static Type? FindEditorForType(Type targetType)
        {
            if (EditorTypes.TryGetValue(targetType, out Type? cached))
                return cached;
            foreach (Assembly assembly in AppDomain.CurrentDomain.GetAssemblies())
            {
                Type[] types;
                try { types = assembly.GetTypes(); }
                catch (ReflectionTypeLoadException ex) { types = ex.Types.OfType<Type>().ToArray(); }
                foreach (Type editorType in types)
                {
                    if (editorType.IsAbstract || !typeof(CustomEditor).IsAssignableFrom(editorType))
                        continue;
                    var attribute = editorType.GetCustomAttribute<CustomEditorAttribute>();
                    if (attribute?.Type == targetType)
                    {
                        EditorTypes[targetType] = editorType;
                        return editorType;
                    }
                }
            }
            EditorTypes[targetType] = null;
            return null;
        }

        private static bool IsNumber(Type type) => Type.GetTypeCode(type) is
            TypeCode.Byte or TypeCode.SByte or TypeCode.Int16 or TypeCode.UInt16 or
            TypeCode.Int32 or TypeCode.UInt32 or TypeCode.Int64 or TypeCode.UInt64 or
            TypeCode.Single or TypeCode.Double or TypeCode.Decimal;
    }
}
