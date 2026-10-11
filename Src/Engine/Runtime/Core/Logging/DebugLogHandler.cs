// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Diagnostics;
using System.Security;
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.Marshalling;
using System.Runtime.CompilerServices;

namespace SE.Log
{
    internal partial class DebugLogHandler : ILogHandler
    {
        /// <summary>
        /// Occurs on sending a log message.
        /// </summary>
        public event LogDelegate SendLog;

        /// <inheritdoc />
        public void LogWrite(LogType logType, string message)
        {
            Internal_LogWrite(logType, message);
        }

        /// <inheritdoc />
        public void LogException(Exception exception, Object context)
        {
            Internal_LogException(exception, Object.GetUnmanagedPtr(context));
        }

        /// <inheritdoc />
        public void Log(LogType logType, Object context, string message)
        {
            if (message == null)
                return;
#if BUILD_RELEASE
            string stackTrace = null;
#else
            string stackTrace = Environment.StackTrace;
#endif
            Internal_Log(logType, message, Object.GetUnmanagedPtr(context), stackTrace);
        }

        internal static void Internal_SendLog(LogType logType, string message, string stackTrace)
        {
            if (message == null)
            {
                return;
            }
            var logger = Debug.Logger;
            if (logger.IsLogTypeAllowed(logType))
            {
                Internal_Log(logType, message, IntPtr.Zero, stackTrace);
            }
        }

        internal static unsafe void RegisterNativeLogCallback()
        {
            Internal_SetLogCallback((IntPtr)(delegate* unmanaged[Cdecl]<LogType, char*, void>)&ReceiveNativeLog);
        }

        internal static void UnregisterNativeLogCallback()
        {
            Internal_SetLogCallback(IntPtr.Zero);
        }

        [UnmanagedCallersOnly(CallConvs = new[] { typeof(CallConvCdecl) })]
        private static unsafe void ReceiveNativeLog(LogType logType, char* message)
        {
            // The native callback can run on any logging thread and its text is only valid during this call.
            if (Debug.Logger.LogHandler is DebugLogHandler handler)
            {
                handler.SendLog?.Invoke(logType, Marshal.PtrToStringUni((IntPtr)message), null, null);
            }
        }

        [LibraryImport("SERuntime", EntryPoint = "Internal_SetLogCallback")]
        private static partial void Internal_SetLogCallback(IntPtr callback);

        internal static void Internal_SendLogException(Exception exception)
        {
            Debug.Logger.LogException(exception);
        }

        [LibraryImport("SERuntime", EntryPoint = "Internal_LogWrite", StringMarshalling = StringMarshalling.Custom, StringMarshallingCustomType = typeof(Interop.StringMarshaller))]
        internal static partial void Internal_LogWrite(LogType level, string msg);

        [LibraryImport("SERuntime", EntryPoint = "Internal_Log", StringMarshalling = StringMarshalling.Custom, StringMarshallingCustomType = typeof(Interop.StringMarshaller))]
        internal static partial void Internal_Log(LogType level, string msg, IntPtr obj, string stackTrace);

        [LibraryImport("SERuntime", EntryPoint = "Internal_LogException", StringMarshalling = StringMarshalling.Custom, StringMarshallingCustomType = typeof(Interop.StringMarshaller))]
        internal static partial void Internal_LogException([MarshalUsing(typeof(Interop.ExceptionMarshaller))] Exception exception, IntPtr obj);

        [SecuritySafeCritical]
        public static string Internal_GetStackTrace()
        {
            var stackTrace = new StackTrace(1, true);
            return stackTrace.ToString();
        }
    }
}
