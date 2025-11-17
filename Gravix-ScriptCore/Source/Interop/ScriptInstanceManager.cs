using System;
using System.Runtime.InteropServices;
using System.Collections.Generic;

namespace GravixEngine.Interop
{
    public static class ScriptInstanceManager
    {
        private static Dictionary<IntPtr, object> instances = new Dictionary<IntPtr, object>();
        private static int nextHandle = 1;

        [UnmanagedCallersOnly]
        public static IntPtr CreateScript(IntPtr instance)
        {
            Console.WriteLine("[C#] CreateScript called!");
            try
            {
                string typeName = Marshal.PtrToStringAnsi(instance);
                Console.WriteLine($"[C#] Creating instance of type: {typeName}");
                Type type = Type.GetType(typeName);
                if (type == null)
                {
                    Console.WriteLine($"[C#] Type '{typeName}' not found.");
                    return IntPtr.Zero;
                }
                object obj = Activator.CreateInstance(type);
                IntPtr handle = new IntPtr(nextHandle++);
                instances[handle] = obj;
                Console.WriteLine($"[C#] Successfully created instance with handle: {handle}");
                return handle;
            }
            catch (Exception e)
            {
                Console.WriteLine($"[C#] Error in CreateScript: {e.Message}");
                return IntPtr.Zero;
            }
        }

        [UnmanagedCallersOnly]
        public static void DestroyScript(IntPtr handle)
        {
            try
            {
                if (instances.ContainsKey(handle))
                {
                    instances.Remove(handle);
                }
                else
                {
                    Console.WriteLine($"Handle '{handle}' not found.");
                }
            }
            catch (Exception e)
            {
                Console.WriteLine($"Error in DestroyScript: {e.Message}");
            }
        }

        public static T GetInstance<T>(IntPtr handle) where T : class
        {
            try
            {
                if (instances.TryGetValue(handle, out object obj))
                {
                    return (T)obj;
                }
                else
                {
                    Console.WriteLine($"Handle '{handle}' not found.");
                    return default(T);
                }
            }
            catch (Exception e)
            {
                Console.WriteLine($"Error in GetInstance: {e.Message}");
                return default(T);
            }
        }
    }
}