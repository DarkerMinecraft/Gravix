#pragma once

#include "Renderer/Generic/Framebuffer.h"

#include "Utils/VulkanTypes.h"
#include "VulkanDevice.h"

namespace Gravix 
{

	class VulkanFramebuffer : public Framebuffer
	{	
	public:
		VulkanFramebuffer(Device* device, const FramebufferSpecification& spec);
		virtual ~VulkanFramebuffer();
	private:
		void Init(const FramebufferSpecification& spec);
	private:
		VulkanDevice* m_Device;

		std::vector<AllocatedImage> m_AllocatedImages;
	};

}