#include "pch.h"
#include "VulkanFramebuffer.h"

namespace Gravix 
{
	VulkanFramebuffer::VulkanFramebuffer(Device* device, const FramebufferSpecification& spec)
		: m_Device(static_cast<VulkanDevice*>(device))
	{
		Init(spec);
	}

	VulkanFramebuffer::~VulkanFramebuffer()
	{
	}

	void VulkanFramebuffer::Init(const FramebufferSpecification& spec)
	{

	}

}