
using System;
using System.Diagnostics;

namespace SE
{
    /// <summary>
    /// Class containing methods to ease debugging while developing a game.
    /// </summary>
    public static class Debug
    {
        internal static readonly Log.Logger _logger = new Log.Logger(new Log.DebugLogHandler());

        /// <summary>
        /// Get default debug logger.
        /// </summary>
        public static Log.ILogger Logger => _logger;
        

        /// <summary>
        /// Assert a condition and logs a formatted error message to the Flax console on failure.
        /// </summary>
        /// <param name="condition">Condition you expect to be true.</param>
        /// <param name="context">Object to which the message applies.</param>
        [Conditional("SE_ASSERTIONS")]
        public static void Assert(bool condition, Object context)
        {
            if (!condition)
            {
                Logger.Log(SE.Log.LogType.Error, (object)"Assertion failed", context);
            }
        }

        /// <summary>
        /// Assert a condition and logs a formatted error message to the Flax console on failure.
        /// </summary>
        /// <param name="condition">Condition you expect to be true.</param>
        /// <param name="message">String or object to be converted to string representation for display.</param>
        [Conditional("SE_ASSERTIONS")]
        public static void Assert(bool condition, object message)
        {
            if (!condition)
                Logger.Log(SE.Log.LogType.Error, message);
        }

        /// <summary>
        /// Assert a condition and logs a formatted error message to the Flax console on failure.
        /// </summary>
        /// <param name="condition">Condition you expect to be true.</param>
        /// <param name="message">String to be converted to string representation for display.</param>
        [Conditional("SE_ASSERTIONS")]
        public static void Assert(bool condition, string message)
        {
            if (!condition)
                Logger.Log(SE.Log.LogType.Error, message);
        }

        /// <summary>
        /// Assert a condition and logs a formatted error message to the Flax console on failure.
        /// </summary>
        /// <param name="condition">Condition you expect to be true.</param>
        /// <param name="context">Object to which the message applies.</param>
        /// <param name="message">String or object to be converted to string representation for display.</param>
        [Conditional("SE_ASSERTIONS")]
        public static void Assert(bool condition, object message, Object context)
        {
            if (!condition)
                Logger.Log(SE.Log.LogType.Error, message, context);
        }

        /// <summary>
        /// Assert a condition and logs a formatted error message to the Flax console on failure.
        /// </summary>
        /// <param name="condition">Condition you expect to be true.</param>
        /// <param name="context">Object to which the message applies.</param>
        /// <param name="message">String to be converted to string representation for display.</param>
        [Conditional("SE_ASSERTIONS")]
        public static void Assert(bool condition, string message, Object context)
        {
            if (!condition)
                Logger.Log(SE.Log.LogType.Error, (object)message, context);
        }

        /// <summary>
        /// Assert a condition and logs a formatted error message to the Flax console on failure.
        /// </summary>
        /// <param name="condition">Condition you expect to be true.</param>
        /// <param name="format">A composite format string.</param>
        /// <param name="args">Format arguments.</param>
        [Conditional("SE_ASSERTIONS")]
        public static void AssertFormat(bool condition, string format, params object[] args)
        {
            if (!condition)
                Logger.LogFormat(SE.Log.LogType.Error, format, args);
        }

        /// <summary>
        /// Assert a condition and logs a formatted error message to the Flax console on failure.
        /// </summary>
        /// <param name="condition">Condition you expect to be true.</param>
        /// <param name="format">A composite format string.</param>
        /// <param name="args">Format arguments.</param>
        /// <param name="context">Object to which the message applies.</param>
        [Conditional("SE_ASSERTIONS")]
        public static void AssertFormat(bool condition, Object context, string format, params object[] args)
        {
            if (!condition)
                Logger.Log(SE.Log.LogType.Error, context, string.Format(format, args));
        }

        /// <summary>
        /// Logs the raw message to the log.
        /// </summary>
        /// <param name="logType">Type of the log message. Not: fatal will stop the engine. Error may show a message popup.</param>
        /// <param name="message">The message contents.</param>
        public static void Write(SE.Log.LogType logType, string message)
        {
            Logger.LogHandler.LogWrite(logType, message);
        }

        /// <summary>
        /// Logs message to the Flax Console.
        /// </summary>
        /// <param name="message">String or object to be converted to string representation for display.</param>
        public static void Log(object message)
        {
            Logger.Log(SE.Log.LogType.Info, message);
        }

        /// <summary>
        /// Logs message to the Flax Console.
        /// </summary>
        /// <param name="message">String or object to be converted to string representation for display.</param>
        /// <param name="context">Object to which the message applies.</param>
        public static void Log(object message, Object context)
        {
            Logger.Log(SE.Log.LogType.Info, message, context);
        }

        /// <summary>
        /// A variant of Debug.Info that logs an assertion message to the console.
        /// </summary>
        /// <param name="message">String or object to be converted to string representation for display.</param>
        [Conditional("SE_ASSERTIONS")]
        public static void LogAssertion(object message)
        {
            Logger.Log(SE.Log.LogType.Error, message);
        }

        /// <summary>
        /// A variant of Debug.Info that logs an assertion message to the console.
        /// </summary>
        /// <param name="message">String or object to be converted to string representation for display.</param>
        /// <param name="context">Object to which the message applies.</param>
        [Conditional("SE_ASSERTIONS")]
        public static void LogAssertion(object message, Object context)
        {
            Logger.Log(SE.Log.LogType.Error, message, context);
        }

        /// <summary>
        /// Logs a formatted assertion message to the Flax console.
        /// </summary>
        /// <param name="format">A composite format string.</param>
        /// <param name="args">Format arguments.</param>
        [Conditional("SE_ASSERTIONS")]
        public static void LogAssertionFormat(string format, params object[] args)
        {
            Logger.LogFormat(SE.Log.LogType.Error, format, args);
        }

        /// <summary>
        /// Logs a formatted assertion message to the Flax console.
        /// </summary>
        /// <param name="format">A composite format string.</param>
        /// <param name="args">Format arguments.</param>
        /// <param name="context">Object to which the message applies.</param>
        [Conditional("SE_ASSERTIONS")]
        public static void LogAssertionFormat(Object context, string format, params object[] args)
        {
            Logger.Log(SE.Log.LogType.Error, context, string.Format(format, args));
        }

        /// <summary>
        /// A variant of Debug.Info that logs an error message to the console.
        /// </summary>
        /// <param name="message">String or object to be converted to string representation for display.</param>
        public static void LogError(object message)
        {
            Logger.Log(SE.Log.LogType.Error, message);
        }

        /// <summary>
        /// A variant of Debug.Info that logs an error message to the console.
        /// </summary>
        /// <param name="message">String or object to be converted to string representation for display.</param>
        /// <param name="context">Object to which the message applies.</param>
        public static void LogError(object message, Object context)
        {
            Logger.Log(SE.Log.LogType.Error, message, context);
        }

        /// <summary>
        /// Logs a formatted error message to the Flax console.
        /// </summary>
        /// <param name="format">A composite format string.</param>
        /// <param name="args">Format arguments.</param>
        public static void LogErrorFormat(string format, params object[] args)
        {
            Logger.LogFormat(SE.Log.LogType.Error, format, args);
        }

        /// <summary>
        /// Logs a formatted error message to the Flax console.
        /// </summary>
        /// <param name="format">A composite format string.</param>
        /// <param name="args">Format arguments.</param>
        /// <param name="context">Object to which the message applies.</param>
        public static void LogErrorFormat(Object context, string format, params object[] args)
        {
            Logger.Log(SE.Log.LogType.Error, context, string.Format(format, args));
        }

        /// <summary>
        /// A variant of Debug.Info that logs an error message to the console.
        /// </summary>
        /// <param name="exception">Runtime Exception.</param>
        public static void LogException(Exception exception)
        {
            Logger.LogException(exception, null);

            if (exception.InnerException != null)
            {
                LogWarning("Inner exception:");
                LogException(exception.InnerException);
            }
        }

        /// <summary>
        /// A variant of Debug.Info that logs an error message to the console.
        /// </summary>
        /// <param name="context">Object to which the message applies.</param>
        /// <param name="exception">Runtime Exception.</param>
        public static void LogException(Exception exception, Object context)
        {
            Logger.LogException(exception, context);

            if (exception.InnerException != null)
            {
                LogWarning("Inner exception:");
                LogException(exception.InnerException, context);
            }
        }

        /// <summary>
        /// Logs a formatted message to the Flax Console.
        /// </summary>
        /// <param name="format">A composite format string.</param>
        /// <param name="args">Format arguments.</param>
        public static void LogFormat(string format, params object[] args)
        {
            Logger.LogFormat(SE.Log.LogType.Info, format, args);
        }

        /// <summary>
        /// Logs a formatted message to the Flax Console.
        /// </summary>
        /// <param name="format">A composite format string.</param>
        /// <param name="args">Format arguments.</param>
        /// <param name="context">Object to which the message applies.</param>
        public static void LogFormat(Object context, string format, params object[] args)
        {
            Logger.Log(SE.Log.LogType.Info, context, string.Format(format, args));
        }

        /// <summary>
        /// A variant of Debug.Info that logs a warning message to the console.
        /// </summary>
        /// <param name="message">String or object to be converted to string representation for display.</param>
        public static void LogWarning(object message)
        {
            Logger.Log(SE.Log.LogType.Warning, message);
        }

        /// <summary>
        /// A variant of Debug.Info that logs a warning message to the console.
        /// </summary>
        /// <param name="message">String or object to be converted to string representation for display.</param>
        /// <param name="context">Object to which the message applies.</param>
        public static void LogWarning(object message, Object context)
        {
            Logger.Log(SE.Log.LogType.Warning, message, context);
        }

        /// <summary>
        /// Logs a formatted warning message to the Flax Console.
        /// </summary>
        /// <param name="format">A composite format string.</param>
        /// <param name="args">Format arguments.</param>
        public static void LogWarningFormat(string format, params object[] args)
        {
            Logger.Log(SE.Log.LogType.Warning, format, args);
        }

        /// <summary>
        /// Logs a formatted warning message to the Flax Console.
        /// </summary>
        /// <param name="format">A composite format string.</param>
        /// <param name="args">Format arguments.</param>
        /// <param name="context">Object to which the message applies.</param>
        public static void LogWarningFormat(Object context, string format, params object[] args)
        {
            Logger.Log(SE.Log.LogType.Warning, context, string.Format(format, args));
        }
    }
}
