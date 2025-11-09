#pragma once

#include "Core.h"
#include "Window.h"
#include "Layer.h"

#include "Events/WindowEvents.h"
#include "Renderer/ImGuiRender.h"

#include "Project/Project.h"

#include <chrono>
#include <filesystem>

namespace Gravix
{

	class Scheduler;

	struct ApplicationSpecification 
	{
		uint32_t Width = 1280;
		uint32_t Height = 720;
		const char* Title = "Gravix Engine";
		bool IsRuntime = false;

		bool VSync = true;
	};

	class Application
	{
	public:
		Application(const ApplicationSpecification& spec = ApplicationSpecification());
		virtual ~Application();

		void Run();
		void Shutdown();

		void OnEvent(Event& event);

        template<typename TLayer>
        requires(std::is_base_of_v<Layer, TLayer>)
        void PushLayer()
        {
            m_LayerStack.push_back(CreateRef<TLayer>());
        }

		bool IsRuntime() { return m_IsRuntime; }

		Window& GetWindow() { return *m_Window; }
		Scheduler& GetScheduler() { return *m_Scheduler; }
		ImGuiRender& GetImGui() { return *m_ImGuiRender; }

		static Application& Get() { return *s_Instance; }
	private:
		bool OnWindowClose(WindowCloseEvent& e);
		bool OnWindowResize(WindowResizeEvent& e);
	private:
		Scope<Window> m_Window;
		Scope<Scheduler> m_Scheduler;

		ImGuiRender* m_ImGuiRender = nullptr;

		bool m_IsRunning = false;
		bool m_IsMinimize = false;
		bool m_IsRuntime = false;

		std::vector<Ref<Layer>> m_LayerStack;
		std::chrono::time_point<std::chrono::high_resolution_clock> m_LastFrameTime;

		static Application* s_Instance;
	};
}