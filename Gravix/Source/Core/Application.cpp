#include "pch.h"
#include "Application.h"

#include "Scheduler.h"

#include "Scene/ComponentRegistry.h"

#include "Scripting/ScriptEngine.h"
#include "Scripting/Interop/ScriptInstance.h"
#include "Asset/EditorAssetManager.h"
#include "Debug/Instrumentor.h"

namespace Gravix
{
	Application* Application::s_Instance = nullptr;

	Application::Application(const ApplicationSpecification& spec)
	{
		GX_PROFILE_FUNCTION();

		s_Instance = this;

		WindowSpecification windowSpec;
		windowSpec.Height = spec.Height;
		windowSpec.Width = spec.Width;
		windowSpec.Title = spec.Title;

		m_Window = Window::Create(windowSpec);
		m_Window->SetEventCallback(BIND_EVENT_FN(OnEvent));

		m_IsRunning = true;
		m_LastFrameTime = std::chrono::high_resolution_clock::now();
		m_IsRuntime = spec.IsRuntime;

		m_Scheduler = CreateScope<Scheduler>();
		m_Scheduler->Init(4); // Initialize with 4 threads

		m_ImGuiRender = new ImGuiRender();
		ComponentRegistry::Get().RegisterAllComponents();

		ScriptEngine::Init("GravixScripting.dll");

		// Test CreateInstance and call instance methods via reflection
		{
			GX_CORE_INFO("[Application] Testing C# Main class with instance methods");

			// Create an instance of Main (calls constructor)
			auto mainInstance = ScriptEngine::CreateInstance("GravixEngine.Main");

			if (mainInstance.IsValid())
			{
				GX_CORE_INFO("[Application] Main instance created successfully");

				// Call all instance methods via reflection
				GX_CORE_INFO("[Application] Calling PrintMessage()");
				mainInstance.Call("PrintMessage");

				GX_CORE_INFO("[Application] Calling PrintInt(42)");
				mainInstance.Call("PrintInt", 42);

				GX_CORE_INFO("[Application] Calling PrintInts(123, 456)");
				mainInstance.Call("PrintInts", 123, 456);

				GX_CORE_INFO("[Application] Calling PrintCustomMessage(\"Hello from C++!\")");
				mainInstance.Call("PrintCustomMessage", "Hello from C++!");
			}
			else
			{
				GX_CORE_ERROR("[Application] Failed to create Main instance");
			}

			GX_CORE_INFO("[Application] Finished testing C# Main class");
		}
	}

	Application::~Application()
	{
		GX_PROFILE_FUNCTION();

		ScriptEngine::Shutdown();
	}

	void Application::Run()
	{
		GX_PROFILE_FUNCTION();

		const float MAX_TIMESTEP = 0.05f; // 50ms max
		std::chrono::time_point<std::chrono::high_resolution_clock> currentTime;

		while (m_IsRunning)
		{
			GX_PROFILE_SCOPE("MainLoop");

			m_Window->GetDevice()->StartFrame();

			currentTime = std::chrono::high_resolution_clock::now();
			std::chrono::duration<float> elapsedTime = currentTime - m_LastFrameTime;
			m_LastFrameTime = currentTime;

			float deltaTime = std::min(elapsedTime.count(), MAX_TIMESTEP);

			if (!m_IsMinimize)
			{
				// Process async asset loads every frame if we have an active project
				{
					GX_PROFILE_SCOPE("ProcessAsyncLoads");
					if (auto activeProject = Project::GetActive())
					{
						if (auto editorAssetManager = activeProject->GetEditorAssetManager())
						{
							editorAssetManager->ProcessAsyncLoads();
						}
					}
				}

				{
					GX_PROFILE_SCOPE("LayerUpdate");
					for (Ref<Layer> layer : m_LayerStack)
						layer->OnUpdate(deltaTime);
				}

				{
					GX_PROFILE_SCOPE("LayerRender");
					for(Ref<Layer> layer : m_LayerStack)
						layer->OnRender();
				}

				if (!m_IsRuntime)
				{
					{
						GX_PROFILE_SCOPE("ImGuiRender");
						m_ImGuiRender->Begin();
						for (Ref<Layer> layer : m_LayerStack)
							layer->OnImGuiRender();
						m_ImGuiRender->End();
					}
				}

			}

			{
				GX_PROFILE_SCOPE("WindowUpdate");
				m_Window->OnUpdate();
			}

			m_Window->GetDevice()->EndFrame();
		}

		{
			GX_PROFILE_SCOPE("ImGuiCleanup");
			delete m_ImGuiRender;
		}
	}

	void Application::Shutdown()
	{
		GX_PROFILE_FUNCTION();

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