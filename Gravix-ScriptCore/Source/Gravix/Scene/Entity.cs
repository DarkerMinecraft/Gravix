using System;

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

        public bool IsValid()
        {
            return ID != 0;
        }

        public static bool operator true(Entity entity)
        {
            return entity != null && entity.ID != 0;
        }

        public static bool operator false(Entity entity)
        {
            return entity == null || entity.ID == 0;
        }

        public static bool operator !(Entity entity)
        {
            return entity == null || entity.ID == 0;
        }

        public static implicit operator bool(Entity entity)
        {
            return entity != null && entity.ID != 0;
        }

        public bool HasComponent<T>() where T : Component, new()
        {
            Type componentType = typeof(T);
            return InternalCalls.Entity_HasComponent(ID, componentType);
        }

        public T GetComponent<T>() where T : Component, new()
        {
            if (!HasComponent<T>())
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

        public Entity FindEntityByName(string name)
        {
            ulong entityID = InternalCalls.Entity_FindEntityByName(name);
            return new Entity(entityID);
        }

        public T As<T>() where T : Entity
        {
            object instance = InternalCalls.Entity_GetScriptInstance(ID, typeof(T));
            return instance as T;
        } 
    }

}