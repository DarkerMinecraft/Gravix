#pragma once

#include "Renderer/Generic/Device.h"

#include <vulkan/vulkan.h>

namespace Gravix 
{

	class VulkanDevice : public Device
	{
	public:
		VulkanDevice(const DeviceProperties& deviceProperties);
	private:
	};

}