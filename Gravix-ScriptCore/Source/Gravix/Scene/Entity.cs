using System;
using System.Collections.Specialized;
using System.Runtime.CompilerServices;

namespace GravixEngine
{
    public class Entity
    {
        protected Entity() 
        {
            ID = 0;
        }

        internal Entity(ulong id)
        {
            ID = id;
        }

        public readonly ulong ID;

        public bool HasComponent<T>() where T : Component, new()
        {
            Type componentType = typeof(T);
            return InternalCalls.Entity_HasComponent(ID, componentType);
        }

        public T GetComponent<T>() where T : Component, new()
        {
            if(!HasComponent<T>())
            {
                throw new InvalidOperationException($"Entity {ID} does not have component of type {typeof(T).Name}");
            }

            T component = new T() { Entity = this };
            return component;
        }

        public T AddComponent<T>() where T : Component, new()
        {
            Type componentType = typeof(T);
            InternalCalls.Entity_AddComponent(ID, componentType);
            return GetComponent<T>();
        }

        public void RemoveComponent<T>() where T : Component
        {
            Type componentType = typeof(T);
            InternalCalls.Entity_RemoveComponent(ID, componentType);
        }
    }

}