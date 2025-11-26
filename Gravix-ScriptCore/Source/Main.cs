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

        public void PrintInt(int value)
        {
            Console.WriteLine($"Integer value: {value}");
        }

        public void PrintInts(int value1, int value2)
        {
            Console.WriteLine($"Integer values: {value1}, {value2}");
        }

        public void PrintCustomMessage(string message)
        {
            Console.WriteLine($"C# says: {message}");
        }

    }

}