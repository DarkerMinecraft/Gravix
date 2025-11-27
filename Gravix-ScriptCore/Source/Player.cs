using System;
using GravixEngine;

namespace Sandbox
{

    public class Player : Entity
    {

        private TransformComponent transform;
        private Rigidbody2DComponent rb2d;

        public float speed = 0.1f;
        public float jumpForce = 5.0f;

        void OnCreate()
        {
            transform = GetComponent<TransformComponent>();
            rb2d = GetComponent<Rigidbody2DComponent>();
        }

        void OnUpdate(float deltaTime)
        {
            Vector2 velocity = Vector2.Zero;

            if (Input.IsKeyDown(Key.W))
                velocity.y = 1.0f;
            if (Input.IsKeyDown(Key.S))
                velocity.y = -1.0f;
            if (Input.IsKeyDown(Key.A))
                velocity.x = -1.0f;
            if (Input.IsKeyDown(Key.D))
                velocity.x = 1.0f;

            velocity *= speed;

            rb2d.ApplyLinearImpulse(velocity, true);

            if (Input.IsKeyPressed(Key.Space))
            {
                rb2d.ApplyLinearImpulse(Vector2.Up * jumpForce, true);
            }
        }
    }

}