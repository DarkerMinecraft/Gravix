#include "pch.h"
#include "HostInstance.h"

#include <hostfxr.h>
#include <nethost.h>
#include <coreclr_delegates.h>

namespace Gravix 
{
	HostInstance::HostInstance(const std::filesystem::path& assemblyPath)
	{
		Initialize(assemblyPath);
	}

	void HostInstance::Initialize(const std::filesystem::path& filePath)
	{
		void* LoadContext = load_library();
	}

}