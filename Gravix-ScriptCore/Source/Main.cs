using System;

namespace Gravix
{

    public class Main
    {
        public float FloatVar { get; set; }

        public Main()
        {
            Console.WriteLine("Main Constructor");
        }

        public void PrintMessage()
        {
            Console.WriteLine("Hello from Main class!");
        }

        public void PrintCustomMessage(string message)
        {
            Console.WriteLine($"C# says: {message}");
        }

    }

}