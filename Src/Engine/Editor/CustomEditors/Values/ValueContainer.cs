using System;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Globalization;
using System.Linq;
using System.Reflection;

namespace SE.Editor
{
    /// <summary>
    /// Values edited by one custom editor. A null member describes the root selection.
    /// </summary>
    [HideInEditor]
    public class ValueContainer : List<object>
    {
        protected bool _hasDefaultValue;
        protected object _defaultValue;
        protected bool _hasReferenceValue;
        protected object _referenceValue;

        public MemberInfo Info { get; }
        public Type Type { get; private set; }
        public bool IsSingleObject => Count == 1;
        public bool HasDifferentValues
        {
            get
            {
                for (int i = 1; i < Count; i++)
                {
                    if (!Equals(this[0], this[i]))
                    {
                        return true;
                    }
                }
                return false;
            }
        }

        public bool HasDifferentTypes
        {
            get
            {
                if (Count < 2)
                {
                    return false;
                }
                Type firstType = GetObjectType(this[0]);
                for (int i = 1; i < Count; i++)
                {
                    if (firstType != GetObjectType(this[i]))
                    {
                        return true;
                    }
                }
                return false;
            }
        }

        public bool HasNull => this.Any(value => value == null);
        public bool IsNull => this.All(value => value == null);
        public bool HasValueType => this.Any(value => value != null && GetObjectType(value).IsValueType);
        public bool IsArray => Type.IsArray;
        public bool IsObsolete { get; }
        public Type[] ValuesTypes => this.Select(GetObjectType).Distinct().ToArray();
        public bool HasReferenceValue => _hasReferenceValue;
        public object ReferenceValue => _referenceValue;
        public bool HasDefaultValue => _hasDefaultValue;
        public object DefaultValue => _defaultValue;

        public bool IsReferenceValueModified
        {
            get
            {
                if (!_hasReferenceValue)
                {
                    return false;
                }
                if (_referenceValue is SceneObject referenceSceneObject && referenceSceneObject.HasPrefabLink)
                {
                    foreach (object value in this)
                    {
                        if (value is SceneObject sceneValue && sceneValue == referenceSceneObject)
                        {
                            continue;
                        }
                        if (value == null || value is SceneObject sceneObject &&
                            sceneObject.PrefabObjectID != referenceSceneObject.PrefabObjectID)
                        {
                            return true;
                        }
                    }
                }
                else
                {
                    foreach (object value in this)
                    {
                        if (!ValueEquals(value, _referenceValue))
                        {
                            return true;
                        }
                    }
                }
                return false;
            }
        }

        public bool IsDefaultValueModified => _hasDefaultValue && this.Any(value => !ValueEquals(value, _defaultValue));

        public ValueContainer(MemberInfo info = null)
        {
            Info = info;
            Type = GetMemberType(info);
            IsObsolete = info?.IsDefined(typeof(ObsoleteAttribute), true) ?? false;
        }

        protected ValueContainer(Type type)
        {
            Type = type;
        }

        public ValueContainer(MemberInfo info, ValueContainer instanceValues) : this(info)
        {
            Capacity = instanceValues.Count;
            foreach (object instance in instanceValues)
            {
                Add(GetMemberValue(instance));
            }

            if (instanceValues._hasDefaultValue)
            {
                _defaultValue = GetMemberValue(instanceValues._defaultValue);
                _hasDefaultValue = true;
            }
            else if (info.GetCustomAttribute<DefaultValueAttribute>() is { } defaultValueAttribute)
            {
                _defaultValue = defaultValueAttribute.Value;
                _hasDefaultValue = true;
                if (_defaultValue != null && _defaultValue.GetType() != Type)
                {
                    if (Type == typeof(sbyte))
                    {
                        _defaultValue = Convert.ToSByte(_defaultValue);
                    }
                    else if (Type == typeof(short))
                    {
                        _defaultValue = Convert.ToInt16(_defaultValue);
                    }
                    else if (Type == typeof(ushort))
                    {
                        _defaultValue = Convert.ToUInt16(_defaultValue);
                    }
                    else if (Type == typeof(uint))
                    {
                        _defaultValue = Convert.ToUInt32(_defaultValue);
                    }
                    else if (Type == typeof(ulong))
                    {
                        _defaultValue = Convert.ToUInt64(_defaultValue);
                    }
                    else if (Type == typeof(long))
                    {
                        _defaultValue = Convert.ToInt64(_defaultValue);
                    }
                }
                else if (_defaultValue == null && Type.IsValueType)
                {
                    _defaultValue = Activator.CreateInstance(Type);
                }
            }

            if (instanceValues._hasReferenceValue)
            {
                if (instanceValues._referenceValue == null && !instanceValues.Type.IsValueType)
                {
                    return;
                }
                try
                {
                    _referenceValue = GetMemberValue(instanceValues._referenceValue);
                    _hasReferenceValue = true;
                }
                catch
                {
                    // Flax ignores an incompatible reference object for this member.
                }
            }
        }

        public ValueContainer(Type customType, ValueContainer other) : base(other)
        {
            Info = other.Info;
            Type = customType;
        }

        public void SetType(Type type) => Type = type;
        public virtual object[] GetAttributes() => Info?.GetCustomAttributes(true).ToArray() ?? Array.Empty<object>();

        public virtual void Refresh(ValueContainer instanceValues)
        {
            ArgumentNullException.ThrowIfNull(instanceValues);
            if (instanceValues.Count != Count)
            {
                throw new ArgumentException("Parent value count differs from this container.", nameof(instanceValues));
            }
            for (int i = 0; i < Count; i++)
            {
                this[i] = GetMemberValue(instanceValues[i]);
            }
        }

        public virtual void Set(ValueContainer instanceValues, object value)
        {
            ArgumentNullException.ThrowIfNull(instanceValues);
            if (instanceValues.Count != Count)
            {
                throw new ArgumentException("Parent value count differs from this container.", nameof(instanceValues));
            }
            if (value is IList list && list.Count == Count && Count > 1 &&
                !Type.IsInstanceOfType(value))
            {
                for (int i = 0; i < Count; i++)
                {
                    SetMemberValue(instanceValues[i], list[i]);
                    this[i] = GetMemberValue(instanceValues[i]);
                }
            }
            else
            {
                for (int i = 0; i < Count; i++)
                {
                    SetMemberValue(instanceValues[i], value);
                    this[i] = GetMemberValue(instanceValues[i]);
                }
            }
        }

        public virtual void Set(ValueContainer instanceValues, ValueContainer values)
        {
            ArgumentNullException.ThrowIfNull(instanceValues);
            ArgumentNullException.ThrowIfNull(values);
            if (instanceValues.Count != Count || values.Count != Count)
            {
                throw new ArgumentException("Value counts differ.");
            }
            for (int i = 0; i < Count; i++)
            {
                SetMemberValue(instanceValues[i], values[i]);
                this[i] = GetMemberValue(instanceValues[i]);
            }
        }

        public virtual void Set(ValueContainer instanceValues)
        {
            ArgumentNullException.ThrowIfNull(instanceValues);
            if (instanceValues.Count != Count)
            {
                throw new ArgumentException("Parent value count differs from this container.", nameof(instanceValues));
            }
            for (int i = 0; i < Count; i++)
            {
                SetMemberValue(instanceValues[i], this[i]);
            }
        }

        public virtual void SetDefaultValue(object value) { _defaultValue = value; _hasDefaultValue = true; }
        public virtual void RefreshDefaultValue(object instanceValue)
        {
            _defaultValue =
            GetMemberValue(instanceValue); _hasDefaultValue = true;
        }
        public virtual void ClearDefaultValue() { _defaultValue = null; _hasDefaultValue = false; }
        public virtual void SetReferenceValue(object value) { _referenceValue = value; _hasReferenceValue = true; }
        public virtual void RefreshReferenceValue(object instanceValue)
        {
            _referenceValue =
            GetMemberValue(instanceValue); _hasReferenceValue = true;
        }
        public virtual void ClearReferenceValue() { _referenceValue = null; _hasReferenceValue = false; }

        private static Type GetObjectType(object value) => value?.GetType() ?? typeof(object);
        private static Type GetMemberType(MemberInfo info)
        {
            return info switch
            {
                FieldInfo field => field.FieldType,
                PropertyInfo property => property.PropertyType,
                _ => typeof(object),
            };
        }

        private object GetMemberValue(object instance)
        {
            return Info switch
            {
                FieldInfo field => field.GetValue(instance),
                PropertyInfo property => property.GetValue(instance),
                _ => instance,
            };
        }

        private void SetMemberValue(object instance, object value)
        {
            switch (Info)
            {
                case FieldInfo field: field.SetValue(instance, value); break;
                case PropertyInfo property: property.SetValue(instance, value); break;
                default: throw new InvalidOperationException("Root selection cannot set a member value.");
            }
        }

        private static bool ValueEquals(object left, object right)
        {
            if (left == null && right is string { Length: 0 })
            {
                return true;
            }
            if (left == right)
            {
                return true;
            }
            if (left == null || right == null)
            {
                return false;
            }
            if (left is SceneObject a && right is SceneObject b && a.HasPrefabLink && b.HasPrefabLink)
            {
                return a.PrefabObjectID == b.PrefabObjectID;
            }
            if (left.GetType() != right.GetType())
            {
                bool IsInteger(object value) => Type.GetTypeCode(value.GetType()) is TypeCode.SByte or TypeCode
                    .Byte or TypeCode.Int16 or TypeCode.UInt16 or TypeCode.Int32 or TypeCode.UInt32 or TypeCode
                        .Int64 or TypeCode.UInt64;
                if (IsInteger(left) && IsInteger(right))
                {
                    return Convert.ToDecimal(left, CultureInfo.CurrentCulture) == Convert.ToDecimal(right,
                        CultureInfo.CurrentCulture);
                }
                if (left is double or float or decimal && right is double or float or decimal)
                {
                    return Mathd.NearEqual(Convert.ToDouble(left, CultureInfo.CurrentCulture), Convert
                        .ToDouble(right, CultureInfo.CurrentCulture));
                }
                return false;
            }
            if (left is IList aList && right is IList bList)
            {
                return aList.Count == bList.Count && Enumerable.Range(0, aList.Count).All(i => ValueEquals(aList[i],
                    bList[i]));
            }
            if (left is IEnumerable aEnumerable && right is IEnumerable bEnumerable)
            {
                IEnumerator aEnumerator = aEnumerable.GetEnumerator();
                IEnumerator bEnumerator = bEnumerable.GetEnumerator();
                using IDisposable aDisposable = aEnumerator as IDisposable;
                using IDisposable bDisposable = bEnumerator as IDisposable;
                while (aEnumerator.MoveNext())
                {
                    if (!bEnumerator.MoveNext() || !ValueEquals(aEnumerator.Current, bEnumerator.Current))
                    {
                        return false;
                    }
                }
                return !bEnumerator.MoveNext();
            }
            if (left.GetType().IsValueType && !left.GetType().IsEnum && !left.GetType().IsPrimitive)
            {
                foreach (FieldInfo field in left.GetType().GetFields(BindingFlags.Instance | BindingFlags.Public))
                {
                    if (!ValueEquals(field.GetValue(left), field.GetValue(right)))
                    {
                        return false;
                    }
                }
                foreach (PropertyInfo property in left.GetType().GetProperties(BindingFlags.Instance | BindingFlags
                    .Public))
                {
                    if (property.CanRead && property.GetIndexParameters().Length == 0 && !ValueEquals(property
                        .GetValue(left), property.GetValue(right)))
                    {
                        return false;
                    }
                }
                return true;
            }
            return left.Equals(right);
        }
    }
}
