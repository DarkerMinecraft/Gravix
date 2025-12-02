using System;

namespace GravixEngine
{
    /// <summary>
    /// Marks a private or protected field to be serialized and shown in the inspector.
    /// Similar to Unity's SerializeField attribute.
    /// </summary>
    [AttributeUsage(AttributeTargets.Field, AllowMultiple = false, Inherited = true)]
    public class SerializeField : Attribute
    {
    }

    /// <summary>
    /// Specifies a range for numeric fields, displayed as a slider in the inspector.
    /// </summary>
    [AttributeUsage(AttributeTargets.Field, AllowMultiple = false, Inherited = true)]
    public class Range : Attribute
    {
        public float min;
        public float max;

        public Range(float min, float max)
        {
            this.min = min;
            this.max = max;
        }
    }
}
