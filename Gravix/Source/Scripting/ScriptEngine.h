#pragma once

namespace Gravix 
{

	class ScriptEngine 
	{
	public:
		static void Initialize();
		static void Shutdown();
	private:
		static void InitMono();
		static void ShudownMono();
	};

}