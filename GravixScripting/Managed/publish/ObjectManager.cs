using System;
using System.Runtime.InteropServices;

namespace Gravix.Interop 
{

    [StructLayout(LayoutKind.Sequential)]
    public struct InvokeResult
    {
        public IntPtr DataPtr;      // Pointer to result data
        public int DataType;        // Type of data (see ResultType enum)
        public int DataSize;        // Size of data in bytes
    }

    public enum ResultType
    {
        None = 0,
        Int32 = 1,
        Int64 = 2,
        Float = 3,
        Double = 4,
        Bool = 5,
        String = 6,
        Object = 7,     // Returns as GCHandle
        Void = 8
    }

    public static class ObjectManager 
    {
        [UnmanagedCallersOnly]
        public static IntPtr CreateObject(IntPtr typeNamePtr) 
        {
            try 
            {
                string typeName = Marshal.PtrToStringAnsi(typeNamePtr);

                Type type = Type.GetType(typeName);
                if (type == null)
                {
                    Console.WriteLine($"[ObjectManager] Type not found: {typeName}");
                    return IntPtr.Zero;
                }

                object instance = Activator.CreateInstance(type);

                // Pin the object using GCHandle to prevent garbage collection
                GCHandle handle = GCHandle.Alloc(instance, GCHandleType.Normal);

                Console.WriteLine($"[ObjectManager] Created object: {typeName}");
                return GCHandle.ToIntPtr(handle);
            }
            catch (Exception ex)
            {
                Console.WriteLine($"[ObjectManager] Error creating object: {ex.Message}");
                return IntPtr.Zero;
            }
        }

        [UnmanagedCallersOnly]
        public static void DestroyObject(IntPtr handlePtr)
        {
            try
            {
                if (handlePtr == IntPtr.Zero)
                    return;

                GCHandle handle = GCHandle.FromIntPtr(handlePtr);

                // Dispose if IDisposable
                if (handle.Target is IDisposable disposable)
                {
                    disposable.Dispose();
                }

                handle.Free();
                Console.WriteLine("[ObjectManager] Destroyed object");
            }
            catch (Exception ex)
            {
                Console.WriteLine($"[ObjectManager] Error destroying object: {ex.Message}");
            }
        }

        [UnmanagedCallersOnly]
        public static IntPtr InvokeMethod(IntPtr handlePtr, IntPtr methodNamePtr, IntPtr argsPtr)
        {
            try
            {
                if (handlePtr == IntPtr.Zero)
                    return IntPtr.Zero;

                GCHandle handle = GCHandle.FromIntPtr(handlePtr);
                object instance = handle.Target;

                string methodName = Marshal.PtrToStringAnsi(methodNamePtr);

                // Use reflection to invoke the method
                var method = instance.GetType().GetMethod(methodName);
                if (method == null)
                {
                    Console.WriteLine($"[ObjectManager] Method not found: {methodName}");
                    return IntPtr.Zero;
                }

                // For now, simple invocation without args
                object result = method.Invoke(instance, null);

                // If the result needs to be returned, handle it appropriately
                // This is simplified - you'd need proper marshaling based on return type

                return IntPtr.Zero;
            }
            catch (Exception ex)
            {
                Console.WriteLine($"[ObjectManager] Error invoking method: {ex.Message}");
                return IntPtr.Zero;
            }
        }

        // Helper to get an object from a handle
        public static object GetObject(IntPtr handlePtr)
        {
            if (handlePtr == IntPtr.Zero)
                return null;

            GCHandle handle = GCHandle.FromIntPtr(handlePtr);
            return handle.Target;
        }
    }

}