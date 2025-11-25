#pragma once

#include "Core/Log.h"

namespace Gravix 
{

	extern "C" void Log(const char* msg) 
	{
		GX_INFO("C# -> {0}", msg);
	}

	struct EngineAPI 
	{
		void (*Log)(const char* msg);
	};
	
}