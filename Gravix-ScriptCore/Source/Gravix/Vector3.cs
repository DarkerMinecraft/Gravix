using System.Numerics;

namespace GravixEngine 
{

    public struct Vector3
    {
        public float x;
        public float y;
        public float z;

        public static Vector3 Zero => new Vector3(0);

        public Vector3(float scaler) 
        {
            this.x = scaler;
            this.y = scaler;
            this.z = scaler;
        }

        public Vector3(float x, float y, float z)
        {
            this.x = x;
            this.y = y;
            this.z = z;
        }

        public Vector3(Vector2 xy, float z) 
        {
            this.x = xy.x;
            this.y = xy.y;
            this.z = z;
        }

        public Vector2 XY
        {
            get 
            {
                return new Vector2(x, y);
            }
            set
            {
                x = value.x;
                y = value.y;
            }
        }

        public static Vector3 operator *(Vector3 a, float b)
        {
            return new Vector3(a.x * b, a.y * b, a.z * b);
        }

        public static Vector3 operator +(Vector3 a, Vector3 b)
        {
            return new Vector3(a.x + b.x, a.y + b.y, a.z + b.z);
        }
    }

}