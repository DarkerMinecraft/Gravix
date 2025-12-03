using System;

namespace GravixEngine
{
    // Base class for all user scripts
    // Similar to Unity's MonoBehaviour
    public abstract class MonoBehaviour : Entity
    {
        // Virtual methods that can be overridden by user scripts
        public virtual void OnCreate() { }
        public virtual void OnUpdate(float deltaTime) { }
    }
}
