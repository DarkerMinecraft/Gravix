using System;

public unsafe struct EngineAPI 
{
    public delagate* unmanged[Cdecl]<string, void> Log;
}

public static class ScriptInterface 
{

    private static EngineAPI api;

    [UnmanagedCallersOnly(EntryPoint = "Init")]
    public static void Init(ref EngineAPI engineAPI) 
    {
        api = engineAPI;
        api.Log("C# Script Interface Initialized.");
    }

}