using System;
using System.Runtime.CompilerServices;

namespace GravixEngine
{
    // Module initializer - runs automatically when the assembly is loaded
    internal static class ModuleInitializer
    {
        [ModuleInitializer]
        internal static void Initialize()
        {
            Console.WriteLine("[C#] Module Initializer executed!");

            // Create an instance of Main to trigger the constructor
            var mainInstance = new Main();
        }
    }

    public class Main
    {

        public float FloatVar { get; set; }

        public Main()
        {
            Console.WriteLine("Main constructor!");
        }

        public void PrintMessage()
        {
            Console.WriteLine("Hello World From C#!");
        }

        public void PrintCustomMessage(string message)
        {
            Console.WriteLine($"C# says: {message}");
        }

    }

}