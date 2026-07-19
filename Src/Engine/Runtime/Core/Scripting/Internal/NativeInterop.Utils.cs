
using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Linq.Expressions;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Runtime.Loader;
using System.Runtime.CompilerServices;
using System.Collections.Concurrent;
using System.IO;
using System.Runtime.InteropServices.JavaScript;
using System.Text;
using System.Threading;

namespace SE.Interop
{
    /// <summary>
    /// Editor utilities and helper functions for System.Type.
    /// </summary>
    public static class NativeInteropUtils
    {
        /// <summary>
        /// Gets all currently loaded assemblies in the runtime.
        /// </summary>
        /// <returns>List of assemblies</returns>
        public static Assembly[] GetAssemblies()
        {
// #if USE_NETCORE
            return AssemblyLoadContext.Default.Assemblies.Concat(NativeInterop.scriptingAssemblyLoadContext.Assemblies).ToArray();
/*#else
            return AppDomain.CurrentDomain.GetAssemblies();
#endif*/
        }
        
        
        /// <summary>
        /// Gets the typename full name.
        /// </summary>
        /// <param name="type">The type.</param>
        /// <returns>The full typename of the type.</returns>
        public static string GetTypeName(this Type type)
        {
            if (type.IsGenericType && type.IsConstructedGenericType)
            {
                // For generic types (eg. Dictionary) FullName returns generic parameter types with fully qualified name so simplify it manually
                var sb = new StringBuilder();
                sb.Append(type.Namespace);
                sb.Append('.');
                sb.Append(type.Name);
                sb.Append('[');
                var genericArgs = type.GetGenericArguments();
                for (var i = 0; i < genericArgs.Length; i++)
                {
                    if (i != 0)
                        sb.Append(',');
                    sb.Append(genericArgs[i].GetTypeName());
                }
                sb.Append(']');
                return sb.ToString();
            }
            return type.FullName;
        }
        
        
        /// <summary>
        /// Gets the array of method parameter types.
        /// </summary>
        /// <param name="method">The method to get it's parameters.</param>
        /// <returns>Method parameters array.</returns>
        public static Type[] GetParameterTypes(this MethodBase method)
        {
            Type[] parameterTypes;
            var parameters = method.GetParameters();
            if (parameters.Length != 0)
            {
                parameterTypes = new Type[parameters.Length];
                for (int i = 0; i < parameters.Length; i++)
                {
                    parameterTypes[i] = parameters[i].ParameterType;
                }
            }
            else
            {
                parameterTypes = Array.Empty<Type>();
            }

            return parameterTypes;
        }

    }

}
