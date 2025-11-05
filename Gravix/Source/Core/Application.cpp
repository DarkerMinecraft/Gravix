#include "pch.h"
#include "Application.h"

#include "Scheduler.h"

#include "Scene/ComponentRegistry.h"

#include "Scripting/ScriptEngine.h"

namespace Gravix
{
	Application* Application::s_Instance = nullptr;

	Application::Application(const ApplicationSpecification& spec)
		: m_Project(spec.GlobalProject)
	{
		s_Instance = this;

		WindowSpecification windowSpec;
		windowSpec.Height = spec.Height;
		windowSpec.Width = spec.Width;
		windowSpec.Title = spec.Title;

		m_Window = Window::Create(windowSpec);
		m_Window->SetEventCallback(BIND_EVENT_FN(OnEvent));

		m_IsRunning = true;
		m_LastFrameTime = std::chrono::high_resolution_clock::now();

		m_Scheduler = CreateScope<Scheduler>();
		m_Scheduler->Init(4); // Initialize with 4 threads

		m_Project.CreateProjectDirectories();

		m_ImGuiRender = new ImGuiRender();
		ComponentRegistry::Get().RegisterAllComponents();

		ScriptEngine::Init("GravixScripting.dll");
	}

	Application::~Application()
	{
		ScriptEngine::Shutdown();
	}

	void Application::Run()
	{
		const float MAX_TIMESTEP = 0.05f; // 50ms max
		std::chrono::time_point<std::chrono::high_resolution_clock> currentTime;

		while (m_IsRunning)
		{
			m_Window->GetDevice()->StartFrame();

			currentTime = std::chrono::high_resolution_clock::now();
			std::chrono::duration<float> elapsedTime = currentTime - m_LastFrameTime;
			m_LastFrameTime = currentTime;

			float deltaTime = std::min(elapsedTime.count(), MAX_TIMESTEP);

			if (!m_IsMinimize)
			{
				{
					for (Ref<Layer> layer : m_LayerStack)
						layer->OnUpdate(deltaTime);
					
					for(Ref<Layer> layer : m_LayerStack)
						layer->OnRender();

					m_ImGuiRender->Begin();
					for (Ref<Layer> layer : m_LayerStack)
						layer->OnImGuiRender();
					m_ImGuiRender->End();
				}

			}

			{
				m_Window->OnUpdate();
			}

			m_Window->GetDevice()->EndFrame();
		}

		delete m_ImGuiRender;
	}

	void Application::Shutdown()
	{		
		m_IsRunning = false;
	}

	void Application::OnEvent(Event& event)
	{
		EventDispatcher dispatcher(event);

		dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(OnWindowClose));
		dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(OnWindowResize));

		m_ImGuiRender->OnEvent(event);
		for (auto it = m_LayerStack.end(); it != m_LayerStack.begin();)
		{
			if (event.Handled) break;
			(*--it)->OnEvent(event);
		}
	}

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		Shutdown();
		return true;
	}

	bool Application::OnWindowResize(WindowResizeEvent& e)
	{
		if (e.GetWidth() <= 0 || e.GetHeight() <= 0)
			m_IsMinimize = true;
		else
			m_IsMinimize = false;

		return m_IsMinimize;
	}
}