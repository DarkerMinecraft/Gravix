#pragma once

namespace Gravix 
{

	class ScriptEngine 
	{
	public:
		static void Init();
		static void Shutdown();
	private:
		static void InitDotNet();
	};

}