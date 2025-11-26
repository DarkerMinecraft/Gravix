using System.Runtime.CompilerServices;

namespace GravixEngine 
{

    public static class InternalCalls
    {
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Entity_GetPosition(ulong entityID, out Vector3 position);
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        internal extern static void Entity_SetPosition(ulong entityID, ref Vector3 position);
    }


}