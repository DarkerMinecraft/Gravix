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
		ImGui::Render();  // Finalize ImGui draw data

		// Only render if ImGui has vertices to draw (optimization)
		ImDrawData* drawData = ImGui::GetDrawData();
		if (drawData && drawData->TotalVtxCount > 0)
		{
			Command cmd;

			cmd.BeginRendering();
			cmd.DrawImGui();
			cmd.EndRendering();
		}

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

		// Load fonts for a professional Unity-like appearance with optimizations
		// Reduce oversampling for better performance (default is 3,1 which is expensive)
		ImFontConfig fontConfig;
		fontConfig.OversampleH = 1;  // Reduced from 3 (66% less memory)
		fontConfig.OversampleV = 1;  // Keep at 1
		fontConfig.PixelSnapH = true; // Snap to pixel grid for sharper text

		// Main UI font - slightly larger for better readability
		ImFont* fontRegular = io.Fonts->AddFontFromFileTTF("Assets/fonts/Roboto-Regular.ttf", 15.0f, &fontConfig);
		io.FontDefault = fontRegular;

		// Bold font for headers and emphasis
		ImFont* fontBold = io.Fonts->AddFontFromFileTTF("Assets/fonts/Roboto-Bold.ttf", 15.0f, &fontConfig);

		// Additional font sizes for various UI elements (optimized - only essential sizes)
		io.Fonts->AddFontFromFileTTF("Assets/fonts/Roboto-Bold.ttf", 18.0f, &fontConfig);  // For bold headers

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

		// ===== UNITY-INSPIRED PROFESSIONAL STYLING =====

		// Rounding and spacing - Unity style with subtle polish
		style.Alpha = 1.0f;
		style.DisabledAlpha = 0.5f;
		style.WindowPadding = ImVec2(10.0f, 10.0f);
		style.WindowRounding = 4.0f;  // Subtle rounding for modern look
		style.WindowBorderSize = 1.0f;
		style.WindowMinSize = ImVec2(32.0f, 32.0f);
		style.WindowTitleAlign = ImVec2(0.5f, 0.5f);  // Center title like Unity
		style.WindowMenuButtonPosition = ImGuiDir_Left;
		style.ChildRounding = 4.0f;
		style.ChildBorderSize = 1.0f;
		style.PopupRounding = 4.0f;
		style.PopupBorderSize = 1.0f;
		style.FramePadding = ImVec2(6.0f, 4.0f);
		style.FrameRounding = 3.0f;
		style.FrameBorderSize = 0.0f;
		style.ItemSpacing = ImVec2(8.0f, 6.0f);
		style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
		style.CellPadding = ImVec2(6.0f, 4.0f);
		style.IndentSpacing = 21.0f;
		style.ColumnsMinSpacing = 6.0f;
		style.ScrollbarSize = 16.0f;
		style.ScrollbarRounding = 9.0f;
		style.GrabMinSize = 12.0f;
		style.GrabRounding = 3.0f;
		style.TabRounding = 4.0f;
		style.TabBorderSize = 0.0f;
		style.ColorButtonPosition = ImGuiDir_Right;
		style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
		style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

		// ===== UNITY COLOR PALETTE =====
		// Carefully crafted to match Unity 2022+ dark theme
		ImVec4* colors = style.Colors;

		// Backgrounds - Unity's signature dark theme
		colors[ImGuiCol_WindowBg] = ImVec4(0.196f, 0.196f, 0.196f, 1.00f);  // #323232
		colors[ImGuiCol_ChildBg] = ImVec4(0.157f, 0.157f, 0.157f, 1.00f);   // #282828
		colors[ImGuiCol_PopupBg] = ImVec4(0.196f, 0.196f, 0.196f, 0.98f);
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.157f, 0.157f, 0.157f, 1.00f);

		// Borders - Unity's subtle borders
		colors[ImGuiCol_Border] = ImVec4(0.098f, 0.098f, 0.098f, 1.00f);    // #191919
		colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

		// Text - Unity's text colors
		colors[ImGuiCol_Text] = ImVec4(0.863f, 0.863f, 0.863f, 1.00f);      // #DCDCDC
		colors[ImGuiCol_TextDisabled] = ImVec4(0.502f, 0.502f, 0.502f, 1.00f);
		colors[ImGuiCol_TextSelectedBg] = ImVec4(0.267f, 0.529f, 0.808f, 0.40f);

		// Title bars - Unity's window title styling
		colors[ImGuiCol_TitleBg] = ImVec4(0.125f, 0.125f, 0.125f, 1.00f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.157f, 0.157f, 0.157f, 1.00f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.125f, 0.125f, 0.125f, 0.95f);

		// Frames (inputs, text fields)
		colors[ImGuiCol_FrameBg] = ImVec4(0.251f, 0.251f, 0.251f, 1.00f);   // #404040
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.294f, 0.294f, 0.294f, 1.00f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.333f, 0.333f, 0.333f, 1.00f);

		// Buttons - Unity's button styling
		colors[ImGuiCol_Button] = ImVec4(0.267f, 0.267f, 0.267f, 1.00f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.349f, 0.349f, 0.349f, 1.00f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.443f, 0.443f, 0.443f, 1.00f);

		// Headers (collapsing headers, tree nodes)
		colors[ImGuiCol_Header] = ImVec4(0.267f, 0.267f, 0.267f, 1.00f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.349f, 0.349f, 0.349f, 1.00f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.384f, 0.384f, 0.384f, 1.00f);

		// Tabs - Unity's tab bar styling
		colors[ImGuiCol_Tab] = ImVec4(0.157f, 0.157f, 0.157f, 1.00f);
		colors[ImGuiCol_TabHovered] = ImVec4(0.267f, 0.529f, 0.808f, 0.80f);  // Unity blue
		colors[ImGuiCol_TabActive] = ImVec4(0.196f, 0.196f, 0.196f, 1.00f);
		colors[ImGuiCol_TabUnfocused] = ImVec4(0.125f, 0.125f, 0.125f, 1.00f);
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.157f, 0.157f, 0.157f, 1.00f);

		// Scrollbars
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.157f, 0.157f, 0.157f, 0.80f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.392f, 0.392f, 0.392f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.478f, 0.478f, 0.478f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.549f, 0.549f, 0.549f, 1.00f);

		// Sliders - Unity blue accent
		colors[ImGuiCol_SliderGrab] = ImVec4(0.267f, 0.529f, 0.808f, 1.00f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.353f, 0.627f, 0.902f, 1.00f);

		// Checkmarks - Unity blue accent
		colors[ImGuiCol_CheckMark] = ImVec4(0.267f, 0.529f, 0.808f, 1.00f);

		// Separators
		colors[ImGuiCol_Separator] = ImVec4(0.098f, 0.098f, 0.098f, 1.00f);
		colors[ImGuiCol_SeparatorHovered] = ImVec4(0.267f, 0.529f, 0.808f, 0.78f);
		colors[ImGuiCol_SeparatorActive] = ImVec4(0.267f, 0.529f, 0.808f, 1.00f);

		// Resize grip
		colors[ImGuiCol_ResizeGrip] = ImVec4(0.267f, 0.267f, 0.267f, 0.25f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.267f, 0.529f, 0.808f, 0.67f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(0.267f, 0.529f, 0.808f, 0.95f);

		// Docking
		colors[ImGuiCol_DockingPreview] = ImVec4(0.267f, 0.529f, 0.808f, 0.40f);
		colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.125f, 0.125f, 0.125f, 1.00f);

		// Tables
		colors[ImGuiCol_TableHeaderBg] = ImVec4(0.251f, 0.251f, 0.251f, 1.00f);
		colors[ImGuiCol_TableBorderStrong] = ImVec4(0.098f, 0.098f, 0.098f, 1.00f);
		colors[ImGuiCol_TableBorderLight] = ImVec4(0.157f, 0.157f, 0.157f, 1.00f);
		colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);

		// Plot colors
		colors[ImGuiCol_PlotLines] = ImVec4(0.612f, 0.612f, 0.612f, 1.00f);
		colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.267f, 0.529f, 0.808f, 1.00f);
		colors[ImGuiCol_PlotHistogram] = ImVec4(0.902f, 0.706f, 0.00f, 1.00f);  // Unity yellow
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.784f, 0.00f, 1.00f);

		// Drag and drop
		colors[ImGuiCol_DragDropTarget] = ImVec4(0.267f, 0.529f, 0.808f, 0.90f);

		// Navigation highlight
		colors[ImGuiCol_NavHighlight] = ImVec4(0.267f, 0.529f, 0.808f, 1.00f);
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
		colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.20f);

		// Modal window dimming
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.60f);
	}

}