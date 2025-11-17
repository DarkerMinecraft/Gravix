using System;
using System.Runtime.InteropServices;
using System.Collections.Generic;
using System.Reflection;

namespace GravixEngine.Interop
{
    public static class ScriptInstanceManager
    {
        private static Dictionary<IntPtr, object> instances = new Dictionary<IntPtr, object>();
        private static int nextHandle = 1;

        [UnmanagedCallersOnly]
        public static IntPtr CreateScript(IntPtr typeNamePtr)
        {
            try
            {
                string typeName = Marshal.PtrToStringAnsi(typeNamePtr);
                Type type = Type.GetType(typeName);
                if (type == null)
                {
                    Console.WriteLine($"[C#] Type '{typeName}' not found.");
                    return IntPtr.Zero;
                }
                object obj = Activator.CreateInstance(type);
                IntPtr handle = new IntPtr(nextHandle++);
                instances[handle] = obj;
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
            }
            catch (Exception e)
            {
                Console.WriteLine($"[C#] Error in DestroyScript: {e.Message}");
            }
        }

        // Generic call instance method - takes instance handle and method name
        // Parameters are passed via reflection by name matching
        [UnmanagedCallersOnly]
        public static void CallInstanceMethod(IntPtr handle, IntPtr methodNamePtr, IntPtr argsPtr, int argCount)
        {
            try
            {
                if (!instances.TryGetValue(handle, out object instance))
                {
                    Console.WriteLine($"[C#] Instance handle {handle} not found.");
                    return;
                }

                string methodName = Marshal.PtrToStringAnsi(methodNamePtr);

                // Marshal arguments from native memory
                // For now, we'll use a simple approach: args are passed as IntPtr array
                // Each element can be an int (stored directly) or string (stored as pointer)
                object[] args = new object[argCount];

                // Get method to determine parameter types
                MethodInfo[] methods = instance.GetType().GetMethods(BindingFlags.Public | BindingFlags.Instance);
                MethodInfo method = null;

                foreach (var m in methods)
                {
                    if (m.Name == methodName && m.GetParameters().Length == argCount)
                    {
                        method = m;
                        break;
                    }
                }

                if (method == null)
                {
                    Console.WriteLine($"[C#] Method '{methodName}' with {argCount} parameters not found on type '{instance.GetType().Name}'.");
                    return;
                }

                // Marshal arguments based on parameter types
                ParameterInfo[] parameters = method.GetParameters();
                IntPtr[] argPointers = new IntPtr[argCount];
                if (argCount > 0)
                {
                    Marshal.Copy(argsPtr, argPointers, 0, argCount);
                }

                for (int i = 0; i < argCount; i++)
                {
                    Type paramType = parameters[i].ParameterType;

                    if (paramType == typeof(int))
                    {
                        args[i] = argPointers[i].ToInt32();
                    }
                    else if (paramType == typeof(string))
                    {
                        args[i] = Marshal.PtrToStringAnsi(argPointers[i]);
                    }
                    else
                    {
                        Console.WriteLine($"[C#] Unsupported parameter type: {paramType.Name}");
                        return;
                    }
                }

                method.Invoke(instance, args);
            }
            catch (Exception e)
            {
                Console.WriteLine($"[C#] Error in CallInstanceMethod: {e.Message}");
            }
        }

        public static T GetInstance<T>(IntPtr handle) where T : class
        {
            if (instances.TryGetValue(handle, out object obj))
            {
                return (T)obj;
            }
            return null;
        }
    }
}
