namespace GravixEngine
{

    public class Input
    {
        public static bool IsKeyDown(Key keyCode)
        {
            return InternalCalls.Input_IsKeyDown(keyCode);
        }

        public static bool IsKeyPressed(Key keyCode)
        {
            return InternalCalls.Input_IsKeyPressed(keyCode);
        }
    }

}