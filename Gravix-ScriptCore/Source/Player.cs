using System;
using GravixEngine;

namespace Sandbox 
{

    public class Player : Entity
    {
        public float speed = 1.0f;

        void OnCreate() 
        {
        }

        void OnUpdate(float deltaTime) 
        {
            Vector3 position = Position;

            Vector3 velocity; 
            if(Input.IsKeyPressed(KeyCode.W))
                velocity.x += 1.0f;
            if(Input.IsKeyPressed(KeyCode.S))
                velocity.x -= 1.0f;
            if(Input.IsKeyPressed(KeyCode.A))
                velocity.z += 1.0f;
            if(Input.IsKeyPressed(KeyCode.D))
                velocity.z -= 1.0f;

            Position = position;
        }
    }

}