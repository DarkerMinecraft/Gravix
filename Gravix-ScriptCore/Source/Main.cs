using System;

namespace GravixEngine
{
    public class Main
    {
        public float FloatVar { get; set; }

        public Main()
        {
            Console.WriteLine("Main constructor!");
        }

        // Instance methods - called via reflection
        public void PrintMessage()
        {
            Console.WriteLine("Hello World From C#!");
        }

        public void PrintInt(int number)
        {
            Console.WriteLine($"C# says: {number}");
        }

        public void PrintInts(int a, int b)
        {
            Console.WriteLine($"C# says: {a} and {b}");
        }

        public void PrintCustomMessage(string message)
        {
            Console.WriteLine($"C# says: {message}");
        }
    }
}