#pragma once

#include "Renderer/CommandImpl.h"

namespace Gravix 
{

	class VulkanCommandImpl : public CommandImpl
	{
	public:
		VulkanCommandImpl(const std::string& debugName);
		virtual ~VulkanCommandImpl();
	};

}