using System;
using System.Collections.Specialized;
using System.Runtime.CompilerServices;

namespace GravixEngine
{

    public struct Vector3
    {
        public float x;
        public float y;
        public float z;

        public Vector3(float x, float y, float z)
        {
            this.x = x;
            this.y = y;
            this.z = z;
        }

        static Vector3 operator*(Vector3 a, float b)
        {
            return new Vector3(a.x * b, a.y * b, a.z * b);
        }
    }

    public class Entity
    {
        void OnCreate() { }
        void OnUpdate(float deltaTime) { }
        void OnDestroy() { }

        protected Entity() 
        {
            ID = 0;
        }

        internal Entity(ulong id)
        {
            ID = id;
            Console.WriteLine("Entity created with ID: " + ID);
        }

        public readonly ulong ID;

        public Vector3 Position 
        {
            get
            {
                InternalCalls.Entity_GetPosition(ID, out Vector3 position);
                return position;
            }

            set
            {
                InternalCalls.Entity_SetPosition(ID, ref value);
            }
        }
    }

}