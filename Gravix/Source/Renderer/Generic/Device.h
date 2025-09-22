#pragma once

namespace Gravix 
{

	struct DeviceProperties
	{
		uint32_t Width;
		uint32_t Height;

		void* WindowHandle;

		bool VSync;
	};

	enum class DeviceType
	{
		None = 0,
		Vulkan = 1,
		DirectX12 = 2,
	};

	class Device 
	{
	public:
		virtual ~Device() = default;

		virtual DeviceType GetType() const = 0;
	};

}