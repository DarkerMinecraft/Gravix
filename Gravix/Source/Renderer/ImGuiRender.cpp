#include "pch.h"
#include "ImGuiRender.h"

#include "Core/Application.h"
#include "Renderer/Vulkan/VulkanDevice.h"

#include "Renderer/Generic/Command.h"

#include <imgui.h>
#include <backends/imgui_impl_vulkan.h>
#include <backends/imgui_impl_win32.h>
#include <ImGuizmo.h>

namespace Gravix 
{

	ImGuiRender::ImGuiRender()
	{
		Init();
	}

	ImGuiRender::~ImGuiRender()
	{
		Application& app = Application::Get();
		VulkanDevice* device = static_cast<VulkanDevice*>(app.GetWindow().GetDevice());

		vkDeviceWaitIdle(device->GetDevice());
		for (auto& framebuffer : device->GetFramebuffers())
		{
			framebuffer->DestroyImGuiDescriptors();
		}
		device->GetFramebuffers().clear();

		for (auto& texture : device->GetTextures()) 
		{
			texture->DestroyImGuiDescriptor();
		}
		device->GetTextures().clear();

		ImGui_ImplWin32_Shutdown();
		ImGui_ImplVulkan_Shutdown();
		ImGui::DestroyContext();
	}

	void ImGuiRender::Begin()
	{
		ImGui_ImplWin32_NewFrame();
		ImGui_ImplVulkan_NewFrame();
		ImGui::NewFrame();
		ImGuizmo::BeginFrame();
	}

	void ImGuiRender::End()
	{
		Command cmd;

		cmd.BeginRendering();
		cmd.DrawImGui();
		cmd.EndRendering();

		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}
	}

	void ImGuiRender::OnEvent(Event& e)
	{
		ImGuiIO& io = ImGui::GetIO();
		if (m_BlockEvents)
		{
			e.Handled |= e.IsInCategory(EventCategoryMouse) & io.WantCaptureMouse;
			e.Handled |= e.IsInCategory(EventCategoryKeyboard) & io.WantCaptureKeyboard;
		}
	}

	void ImGuiRender::Init()
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

		SetTheme();

		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		ImFont* font = io.Fonts->AddFontFromFileTTF("Assets/fonts/Roboto-Regular.ttf", 16.0f);
		io.FontDefault = font;

		Application& app = Application::Get();
		HWND window = static_cast<HWND>(app.GetWindow().GetWindowHandle());
		
		//io.IniFilename = imguiIniPath.c_str();

		ImGui_ImplWin32_Init(window);

		VulkanDevice* device = static_cast<VulkanDevice*>(app.GetWindow().GetDevice());

		ImGui_ImplVulkan_InitInfo initInfo{};

		initInfo.Instance = device->GetInstance();
		initInfo.PhysicalDevice = device->GetPhysicalDevice();
		initInfo.Device = device->GetDevice();
		initInfo.Queue = device->GetGraphicsQueue();
		initInfo.DescriptorPool = device->GetImGuiDescriptorPool();
		initInfo.MinImageCount = 3;
		initInfo.ImageCount = 3;
		initInfo.UseDynamicRendering = true;

		initInfo.PipelineRenderingCreateInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
		initInfo.PipelineRenderingCreateInfo.colorAttachmentCount = 1;

		VkFormat format = VK_FORMAT_B8G8R8A8_UNORM;
		initInfo.PipelineRenderingCreateInfo.pColorAttachmentFormats = &format;

		initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

		ImGui::GetPlatformIO().Platform_CreateVkSurface = [](ImGuiViewport* viewport, ImU64 vk_instance, const void* vk_allocator, ImU64* out_vk_surface) -> int
			{
				VkWin32SurfaceCreateInfoKHR createInfo = {};
				createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
				createInfo.hwnd = (HWND)viewport->PlatformHandle;
				createInfo.hinstance = GetModuleHandle(nullptr);

				VkResult err = vkCreateWin32SurfaceKHR(
					(VkInstance)vk_instance,
					&createInfo,
					(const VkAllocationCallbacks*)vk_allocator,
					(VkSurfaceKHR*)out_vk_surface);

				return (err == VK_SUCCESS) ? 1 : 0;
			};

		ImGui_ImplVulkan_Init(&initInfo);
	}

	void ImGuiRender::SetTheme()
	{
		ImGuiStyle& style = ImGui::GetStyle();

		// Rounding and spacing
		style.Alpha = 1.0f;
		style.DisabledAlpha = 0.5f;
		style.WindowPadding = ImVec2(8.0f, 8.0f);
		style.WindowRounding = 0.0f;  // Unity uses sharp corners
		style.WindowBorderSize = 1.0f;
		style.WindowMinSize = ImVec2(32.0f, 32.0f);
		style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
		style.WindowMenuButtonPosition = ImGuiDir_Left;
		style.ChildRounding = 0.0f;
		style.ChildBorderSize = 1.0f;
		style.PopupRounding = 0.0f;
		style.PopupBorderSize = 1.0f;
		style.FramePadding = ImVec2(4.0f, 3.0f);
		style.FrameRounding = 2.0f;  // Subtle rounding
		style.FrameBorderSize = 0.0f;
		style.ItemSpacing = ImVec2(8.0f, 4.0f);
		style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
		style.CellPadding = ImVec2(4.0f, 2.0f);
		style.IndentSpacing = 21.0f;
		style.ColumnsMinSpacing = 6.0f;
		style.ScrollbarSize = 14.0f;
		style.ScrollbarRounding = 0.0f;
		style.GrabMinSize = 10.0f;
		style.GrabRounding = 2.0f;
		style.TabRounding = 0.0f;
		style.TabBorderSize = 0.0f;
		style.ColorButtonPosition = ImGuiDir_Right;
		style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
		style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

		// Unity-inspired color palette
		ImVec4* colors = style.Colors;

		// Backgrounds - Dark gray with slight blue tint
		colors[ImGuiCol_WindowBg] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
		colors[ImGuiCol_ChildBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.98f);
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);

		// Borders
		colors[ImGuiCol_Border] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
		colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

		// Text
		colors[ImGuiCol_Text] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
		colors[ImGuiCol_TextSelectedBg] = ImVec4(0.23f, 0.45f, 0.69f, 0.54f);

		// Title bars
		colors[ImGuiCol_TitleBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.15f, 0.15f, 0.15f, 0.95f);

		// Frames (inputs, etc)
		colors[ImGuiCol_FrameBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);

		// Buttons
		colors[ImGuiCol_Button] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);

		// Headers (collapsing headers, trees)
		colors[ImGuiCol_Header] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);

		// Tabs
		colors[ImGuiCol_Tab] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
		colors[ImGuiCol_TabHovered] = ImVec4(0.32f, 0.32f, 0.32f, 1.00f);
		colors[ImGuiCol_TabActive] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
		colors[ImGuiCol_TabUnfocused] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);

		// Scrollbars
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.45f, 0.45f, 0.45f, 1.00f);

		// Sliders
		colors[ImGuiCol_SliderGrab] = ImVec4(0.44f, 0.58f, 0.80f, 1.00f);  // Unity blue accent
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.52f, 0.67f, 0.90f, 1.00f);

		// Checkmarks
		colors[ImGuiCol_CheckMark] = ImVec4(0.44f, 0.58f, 0.80f, 1.00f);  // Unity blue accent

		// Separators
		colors[ImGuiCol_Separator] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
		colors[ImGuiCol_SeparatorHovered] = ImVec4(0.40f, 0.52f, 0.70f, 1.00f);
		colors[ImGuiCol_SeparatorActive] = ImVec4(0.44f, 0.58f, 0.80f, 1.00f);

		// Resize grip
		colors[ImGuiCol_ResizeGrip] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.44f, 0.58f, 0.80f, 0.67f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(0.52f, 0.67f, 0.90f, 0.95f);

		// Tables
		colors[ImGuiCol_TableHeaderBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
		colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
		colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
		colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.03f);

		// Plot colors
		colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
		colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.68f, 0.80f, 1.00f, 1.00f);
		colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.80f, 0.00f, 1.00f);

		// Drag and drop
		colors[ImGuiCol_DragDropTarget] = ImVec4(0.44f, 0.58f, 0.80f, 0.90f);

		// Navigation
		colors[ImGuiCol_NavHighlight] = ImVec4(0.44f, 0.58f, 0.80f, 1.00f);
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
		colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);

		// Modal windows
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.60f);
	}

}