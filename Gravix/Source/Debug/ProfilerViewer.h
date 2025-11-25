#pragma once

#ifdef ENGINE_DEBUG

#include "Instrumentor.h"
#include <unordered_map>
#include <algorithm>
#include <numeric>

namespace Gravix
{
	/**
	 * @brief ImGui-based real-time profiler viewer
	 *
	 * Displays profiling data from the Instrumentor in a live ImGui window.
	 * Shows per-function timings, frame time graphs, and performance statistics.
	 * Only available in debug builds.
	 */
	class ProfilerViewer
	{
	public:
		ProfilerViewer();
		~ProfilerViewer();

		/**
		 * @brief Render the profiler ImGui window
		 *
		 * Should be called each frame during ImGui rendering phase.
		 * @param deltaTime Time since last frame in seconds
		 */
		void OnImGuiRender(float deltaTime);

		/**
		 * @brief Show or hide the profiler window
		 */
		void SetVisible(bool visible) { m_Visible = visible; }
		bool IsVisible() const { return m_Visible; }

	private:
		void RenderFrameStats();
		void RenderFunctionTimings();
		void RenderFrameGraph();

		struct FunctionStats
		{
			float TotalTime = 0.0f;
			float MinTime = FLT_MAX;
			float MaxTime = 0.0f;
			uint32_t CallCount = 0;

			float GetAverage() const { return CallCount > 0 ? TotalTime / CallCount : 0.0f; }
		};

	private:
		bool m_Visible = false;
		float m_FrameTime = 0.0f;
		float m_FPS = 0.0f;

		// Display values (updated at interval)
		float m_DisplayFrameTime = 0.0f;
		float m_DisplayFPS = 0.0f;

		// Frame time history for graph
		static constexpr int HISTORY_SIZE = 120;
		float m_FrameTimeHistory[HISTORY_SIZE] = {};
		int m_HistoryOffset = 0;

		// Function statistics
		std::unordered_map<std::string, FunctionStats> m_FunctionStats;

		// Update rate control (update display every N seconds)
		float m_UpdateInterval = 1.0f; // Update every second
		float m_TimeSinceLastUpdate = 0.0f;
		std::unordered_map<std::string, FunctionStats> m_DisplayStats; // Stats shown on screen

		// Sorting and filtering
		int m_SortMode = 0; // 0=Total, 1=Average, 2=Max, 3=Calls
		char m_FilterBuffer[256] = "";

		// Display settings
		bool m_ShowGraph = true;
		bool m_ShowFunctions = true;
		float m_GraphHeight = 80.0f;
	};
}

#endif // ENGINE_DEBUG
