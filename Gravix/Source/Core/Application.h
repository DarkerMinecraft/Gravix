#pragma once

#include "Core.h"
#include "Window.h"
#include "Layer.h"

#include "Events/WindowEvents.h"

#ifdef GRAVIX_EDITOR_BUILD
#include "Renderer/ImGuiRender.h"
#endif

#include "Project/Project.h"

#if defined(ENGINE_DEBUG) && defined(GRAVIX_EDITOR_BUILD)
#include "Debug/ProfilerViewer.h"
#endif

#include <chrono>
#include <filesystem>

namespace Gravix
{

	class Scheduler;

	/**
	 * @brief Specification for application initialization
	 *
	 * Contains all configuration parameters needed to create and initialize
	 * the application, including window size, title, and runtime mode.
	 */
	struct ApplicationSpecification
	{
		uint32_t Width = 1280;          ///< Initial window width in pixels
		uint32_t Height = 720;          ///< Initial window height in pixels
		const char* Title = "Gravix Engine"; ///< Window title
		bool IsRuntime = false;         ///< Runtime mode (packaged game) vs editor mode

		bool VSync = true;              ///< Enable vertical synchronization
	};

	/**
	 * @brief Main application class managing the engine lifecycle
	 *
	 * The Application class is the core of the Gravix engine, managing:
	 * - Main application loop (Run)
	 * - Window creation and event handling
	 * - Layer stack for modular update/render systems
	 * - Multi-threaded task scheduler
	 * - ImGui integration for UI
	 *
	 * This class follows the singleton pattern - only one instance can exist.
	 * Access the instance globally via Application::Get().
	 *
	 * @par Usage Example:
	 * @code
	 * ApplicationSpecification spec;
	 * spec.Width = 1920;
	 * spec.Height = 1080;
	 * spec.Title = "My Game";
	 *
	 * Application app(spec);
	 * app.PushLayer<GameLayer>();
	 * app.Run();
	 * @endcode
	 */
	class Application
	{
	public:
		/**
		 * @brief Construct the application with given specification
		 * @param spec Configuration parameters for the application
		 */
		Application(const ApplicationSpecification& spec = ApplicationSpecification());

		/**
		 * @brief Destructor - cleans up all engine systems
		 */
		virtual ~Application();

		/**
		 * @brief Start the main application loop
		 *
		 * Enters the main loop which:
		 * 1. Processes window events
		 * 2. Updates all layers with delta time
		 * 3. Renders all layers
		 * 4. Updates ImGui
		 *
		 * This method blocks until the application is closed.
		 */
		void Run();

		/**
		 * @brief Request application shutdown
		 *
		 * Signals the main loop to exit gracefully after the current frame.
		 */
		void Shutdown();

		/**
		 * @brief Dispatch an event to all layers in the stack
		 * @param event Event to dispatch (window, keyboard, mouse, etc.)
		 *
		 * Events are propagated through the layer stack until handled.
		 */
		void OnEvent(Event& event);

		/**
		 * @brief Push a new layer onto the layer stack
		 * @tparam TLayer Layer type (must derive from Layer)
		 *
		 * The layer will be created and added to the stack, receiving
		 * OnUpdate, OnRender, and OnEvent calls each frame.
		 */
        template<typename TLayer>
        requires(std::is_base_of_v<Layer, TLayer>)
        void PushLayer()
        {
            m_LayerStack.push_back(CreateRef<TLayer>());
        }

		/**
		 * @brief Check if running in runtime mode
		 * @return true if packaged runtime, false if editor mode
		 */
		bool IsRuntime() { return m_IsRuntime; }

		/**
		 * @brief Get the application window
		 * @return Reference to the main window
		 */
		Window& GetWindow() { return *m_Window; }

		/**
		 * @brief Get the task scheduler
		 * @return Reference to the multi-threaded scheduler (enkiTS)
		 */
		Scheduler& GetScheduler() { return *m_Scheduler; }

#ifdef GRAVIX_EDITOR_BUILD
		/**
		 * @brief Get the ImGui renderer (Editor only)
		 * @return Reference to the ImGui rendering system
		 */
		ImGuiRender& GetImGui() { return *m_ImGuiRender; }
#endif

#if defined(ENGINE_DEBUG) && defined(GRAVIX_EDITOR_BUILD)
		/**
		 * @brief Get the profiler viewer (Debug editor builds only)
		 * @return Reference to the real-time profiler viewer
		 */
		ProfilerViewer& GetProfiler() { return *m_ProfilerViewer; }
#endif

		/**
		 * @brief Get the global application instance
		 * @return Reference to the singleton application
		 */
		static Application& Get() { return *s_Instance; }
	private:
		bool OnWindowClose(WindowCloseEvent& e);
		bool OnWindowResize(WindowResizeEvent& e);
	private:
		Scope<Window> m_Window;
		Scope<Scheduler> m_Scheduler;

#ifdef GRAVIX_EDITOR_BUILD
		Ref<ImGuiRender> m_ImGuiRender;
#endif

#if defined(ENGINE_DEBUG) && defined(GRAVIX_EDITOR_BUILD)
		Scope<ProfilerViewer> m_ProfilerViewer;
#endif

		bool m_IsRunning = false;
		bool m_IsMinimize = false;
		bool m_IsRuntime = false;

		std::vector<Ref<Layer>> m_LayerStack;
		std::chrono::time_point<std::chrono::high_resolution_clock> m_LastFrameTime;

		static Application* s_Instance;
	};
}