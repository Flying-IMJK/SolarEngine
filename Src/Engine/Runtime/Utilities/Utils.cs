using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;

namespace SE
{
    /// <summary>
    /// Class with helper functions.
    /// </summary>
    public static partial class Utils
    {


        /// <summary>
        /// Initializes the structure (value type) by inflating it with values from <see cref="System.ComponentModel.DefaultValueAttribute"/> (recursive).
        /// </summary>
        /// <param name="obj">The object to initialize.</param>
        /// <param name="type">The structure type.</param>
        public static void InitStructure(object obj, Type type)
        {
            var fields = type.GetFields(BindingFlags.Default | BindingFlags.Instance | BindingFlags.Public);
            for (var i = 0; i < fields.Length; i++)
            {
                var field = fields[i];
                var fieldType = field.FieldType;
                var attr = field.GetCustomAttribute<System.ComponentModel.DefaultValueAttribute>();
                if (attr != null)
                {
                    var value = attr.Value;
                    if (value != null && value.GetType() != fieldType)
                        value = Convert.ChangeType(value, fieldType);
                    field.SetValue(obj, value);
                }
                else if (fieldType.IsValueType)
                {
                    var fieldValue = Activator.CreateInstance(fieldType);
                    InitStructure(fieldValue, fieldType);
                    field.SetValue(obj, fieldValue);
                }
            }
        }
    }

}
