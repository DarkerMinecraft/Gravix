using System;

namespace GravixEngine
{
    /// <summary>
    /// Debug logging utilities for scripts (similar to Unity's Debug class)
    /// </summary>
    public static class Debug
    {
        /// <summary>
        /// Logs a message to the console
        /// </summary>
        public static void Log(object message)
        {
            if (message == null)
            {
                InternalCalls.Debug_Log("null");
                return;
            }
            InternalCalls.Debug_Log(message.ToString());
        }

        /// <summary>
        /// Logs a warning message to the console
        /// </summary>
        public static void LogWarning(object message)
        {
            if (message == null)
            {
                InternalCalls.Debug_LogWarning("null");
                return;
            }
            InternalCalls.Debug_LogWarning(message.ToString());
        }

        /// <summary>
        /// Logs an error message to the console
        /// </summary>
        public static void LogError(object message)
        {
            if (message == null)
            {
                InternalCalls.Debug_LogError("null");
                return;
            }
            InternalCalls.Debug_LogError(message.ToString());
        }

        /// <summary>
        /// Logs a formatted message to the console
        /// </summary>
        public static void LogFormat(string format, params object[] args)
        {
            try
            {
                string message = string.Format(format, args);
                Log(message);
            }
            catch (FormatException e)
            {
                LogError("String.Format failed: " + e.Message);
            }
        }
    }
}
